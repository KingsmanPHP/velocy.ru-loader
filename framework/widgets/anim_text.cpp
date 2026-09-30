

#include "../headers/includes.h" // var->lang.lang
#include <imgui.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace
{
    AnimTextConfig g_cfg;

    // Fallback "time" если вдруг нет ImGui контекста
    double g_fallback_time = 0.0;
    int    g_fallback_frame = 0;

    static uint32_t fnv1a32(const void* data, size_t len)
    {
        const uint8_t* p = (const uint8_t*)data;
        uint32_t h = 2166136261u;
        for (size_t i = 0; i < len; ++i)
        {
            h ^= (uint32_t)p[i];
            h *= 16777619u;
        }
        return h;
    }

    static uint32_t fnv1a32_str(const char* s)
    {
        if (!s) return 0u;
        return fnv1a32(s, std::strlen(s));
    }

    static uint32_t xorshift32(uint32_t& s)
    {
        // классика, достаточно для визуального эффекта
        s ^= (s << 13);
        s ^= (s >> 17);
        s ^= (s << 5);
        return s;
    }

    static float rand01(uint32_t& s)
    {
        // [0,1)
        uint32_t v = xorshift32(s);
        return (float)(v & 0x00FFFFFFu) / (float)0x01000000u;
    }

    // Разбиваем UTF-8 строку на "глифы" (каждый элемент — один UTF-8 кодпоинт как кусок bytes).
    // Это решает проблему русских букв: мы никогда не режем посреди многобайтового символа.
    static std::vector<std::string> utf8_split(const std::string& s)
    {
        std::vector<std::string> out;
        out.reserve(s.size()); // верхняя оценка

        const size_t n = s.size();
        size_t i = 0;
        while (i < n)
        {
            unsigned char c = (unsigned char)s[i];
            size_t clen = 1;

            if (c < 0x80) clen = 1;
            else if ((c & 0xE0) == 0xC0) clen = 2;
            else if ((c & 0xF0) == 0xE0) clen = 3;
            else if ((c & 0xF8) == 0xF0) clen = 4;
            else clen = 1; // битый байт — считаем одиночным

            if (i + clen > n) clen = 1;

            out.emplace_back(s.substr(i, clen));
            i += clen;
        }

        return out;
    }

    // ВАЖНО: НЕ используем '#', чтобы случайно не собрать "##" в тексте (у ImGui это спец-разделитель ID).
    // См. "Label##foobar" в доке/FAQ по ID стеку. :contentReference[oaicite:3]{index=3}
    static constexpr char kNoiseTable[] =
        "!@$%^&*()_+-=[]{}|;:,.<>?/\\~"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    static char noise_char(uint32_t base_seed, size_t index, int step)
    {
        uint32_t h = base_seed;
        h ^= (uint32_t)index * 0x9E3779B9u;
        h ^= (uint32_t)step  * 0x85EBCA6Bu;
        xorshift32(h);

        constexpr size_t table_len = sizeof(kNoiseTable) - 1;
        return kNoiseTable[h % table_len];
    }

    struct AnimState
    {
        bool initialized = false;

        std::string current;  // кэш итоговой строки (для текущего кадра)
        std::string target;   // текущий целевой текст (последний принятый)

        // Анимация
        bool  animating = false;
        float progress  = 1.0f; // 0..1
        int   last_frame_updated = -1;

        int   scramble_step = 0;
        float scramble_accum = 0.0f;

        // Подготовленные данные перехода
        std::vector<std::string> from_glyphs;
        std::vector<std::string> to_glyphs;
        std::vector<float> hide_t; // пороги скрытия (фаза 1)
        std::vector<float> show_t; // пороги раскрытия (фаза 2)

        uint32_t noise_seed = 0;

        // GC
        double last_used_time = 0.0;
    };

    static std::unordered_map<uint32_t, AnimState> g_states;
    static double g_last_gc_time = 0.0;

    static void gc_states(double now)
    {
        if (g_cfg.gc_forget_after <= 0.0f) return;
        if (g_cfg.gc_interval <= 0.0f) return;
        if (now - g_last_gc_time < (double)g_cfg.gc_interval) return;

        g_last_gc_time = now;

        for (auto it = g_states.begin(); it != g_states.end(); )
        {
            AnimState& st = it->second;
            const double age = now - st.last_used_time;

            if (!st.animating && age > (double)g_cfg.gc_forget_after)
                it = g_states.erase(it);
            else
                ++it;
        }
    }

    static std::string build_display(const AnimState& st)
    {
        const float p = st.progress;

        const size_t from_len = st.from_glyphs.size();
        const size_t to_len   = st.to_glyphs.size();
        const size_t n        = (std::max)(from_len, to_len);

        std::string out;
        // грубый reserve, чтобы не аллоцировать по 100 раз:
        out.reserve(st.target.size() + 16);

        if (p < 0.5f)
        {
            // Фаза 1: старое -> шум
            const float p1 = (p * 2.0f); // 0..1

            for (size_t i = 0; i < n; ++i)
            {
                const std::string* from = (i < from_len) ? &st.from_glyphs[i] : nullptr;
                const std::string* to   = (i < to_len)   ? &st.to_glyphs[i]   : nullptr;

                if (g_cfg.keep_spaces)
                {
                    if ((from && *from == " ") || (to && *to == " "))
                    {
                        out.push_back(' ');
                        continue;
                    }
                }

                const bool keep_old = (from && (p1 < st.hide_t[i]));
                if (keep_old)
                {
                    out += *from;
                }
                else
                {
                    out.push_back(noise_char(st.noise_seed, i, st.scramble_step));
                }
            }
        }
        else
        {
            // Фаза 2: шум -> новое
            const float p2 = (p - 0.5f) * 2.0f; // 0..1

            for (size_t i = 0; i < n; ++i)
            {
                const std::string* from = (i < from_len) ? &st.from_glyphs[i] : nullptr;
                const std::string* to   = (i < to_len)   ? &st.to_glyphs[i]   : nullptr;

                if (g_cfg.keep_spaces)
                {
                    if ((from && *from == " ") || (to && *to == " "))
                    {
                        out.push_back(' ');
                        continue;
                    }
                }

                const bool show_new = (p2 >= st.show_t[i]);
                if (show_new)
                {
                    if (to) out += *to; // если новый текст короче — тут может быть nullptr => ничего не добавляем
                }
                else
                {
                    out.push_back(noise_char(st.noise_seed, i, st.scramble_step));
                }
            }
        }

        return out;
    }

    static void begin_transition(AnimState& st, uint32_t key, const std::string& new_target, int frame_count)
    {
        // От чего анимируем:
        // берём то, что реально было видно (current), а если пусто — предыдущую target.
        std::string from_text = !st.current.empty() ? st.current : st.target;

        st.from_glyphs = utf8_split(from_text);
        st.to_glyphs   = utf8_split(new_target);

        const size_t n = (std::max)(st.from_glyphs.size(), st.to_glyphs.size());

        st.hide_t.assign(n, 0.0f);
        st.show_t.assign(n, 0.0f);

        // Детерминированный RNG на переход (чтобы не дрожало каждый кадр)
        uint32_t rng = key ^ 0xA341316Cu ^ (uint32_t)frame_count * 0x9E3779B9u;
        if (rng == 0) rng = 0x12345678u;

        for (size_t i = 0; i < n; ++i)
        {
            st.hide_t[i] = rand01(rng);
            st.show_t[i] = rand01(rng);
        }

        // База для шума (тоже детерминированная)
        st.noise_seed = key ^ 0xC001D00Du;

        st.target    = new_target;
        st.animating = true;
        st.progress  = 0.0f;

        st.scramble_step  = 0;
        st.scramble_accum = 0.0f;

        // чтобы в этом же кадре пересчиталось корректно
        st.last_frame_updated = -1;
    }

    static uint32_t make_key_from_id(const char* id_str)
    {
        if (ImGui::GetCurrentContext())
        {
            // GetID учитывает ID-стек (PushID/PopID) и окно. Это удобно для ImGui.
            // Док: "GetID(...) hash of whole ID stack + given parameter". :contentReference[oaicite:4]{index=4}
            return (uint32_t)ImGui::GetID(id_str);
        }
        // fallback без ImGui контекста
        return fnv1a32_str(id_str);
    }

    static void get_imgui_timing(float& dt, int& frame_count, double& now)
    {
        if (ImGui::GetCurrentContext())
        {
            dt = ImGui::GetIO().DeltaTime;      // seconds since last frame :contentReference[oaicite:5]{index=5}
            frame_count = ImGui::GetFrameCount(); // increments every frame :contentReference[oaicite:6]{index=6}
            now = ImGui::GetTime();             // global imgui time (accumulates DeltaTime) :contentReference[oaicite:7]{index=7}
            if (!(dt > 0.0f)) dt = 1.0f / 60.0f;
            return;
        }

        // no imgui context
        dt = 1.0f / 60.0f;
        g_fallback_time += dt;
        now = g_fallback_time;
        frame_count = ++g_fallback_frame;
    }

    static std::string anim_text_target_internal(const std::string& target, const char* id)
    {
        float dt = 1.0f / 60.0f;
        int frame_count = 0;
        double now = 0.0;
        get_imgui_timing(dt, frame_count, now);

        // GC
        gc_states(now);

        // Делаем стабильный id, даже если id == nullptr
        std::string fallback_key;
        const char* id_str = id;

        if (!id_str || !*id_str)
        {
            // Если юзер не дал ID — ключом будет сам target (как минимум стабильно для этого места кода).
            fallback_key = target;
            id_str = fallback_key.c_str();
        }

        const uint32_t key = make_key_from_id(id_str);

        AnimState& st = g_states[key];
        st.last_used_time = now;

        // 1) Первое появление — без анимации (чтобы UI не шумел при открытии меню)
        if (!st.initialized)
        {
            st.initialized = true;
            st.target = target;
            st.current = target;
            st.animating = false;
            st.progress = 1.0f;
            st.last_frame_updated = frame_count;
            return st.current;
        }

        // 2) Если цель изменилась — запускаем переход
        if (st.target != target)
        {
            begin_transition(st, key, target, frame_count);
        }

        // 3) Обновляем строго 1 раз на кадр
        if (st.last_frame_updated == frame_count)
            return st.current;

        st.last_frame_updated = frame_count;

        // 4) Если не анимируем — просто возвращаем target
        if (!st.animating || g_cfg.duration <= 0.0f)
        {
            st.animating = false;
            st.progress = 1.0f;
            st.current = st.target;
            return st.current;
        }

        // 5) Продвигаем прогресс по dt
        st.progress += dt / g_cfg.duration;
        if (st.progress >= 1.0f)
        {
            st.progress = 1.0f;
            st.animating = false;
            st.current = st.target;
            return st.current;
        }

        // 6) Апдейт шага "шума" с частотой scramble_fps
        if (g_cfg.scramble_fps > 0.0f)
        {
            const float period = 1.0f / g_cfg.scramble_fps;
            st.scramble_accum += dt;
            while (st.scramble_accum >= period)
            {
                st.scramble_accum -= period;
                st.scramble_step++;
            }
        }

        st.current = build_display(st);
        return st.current;
    }
} // namespace

void anim_text_set_config(const AnimTextConfig& cfg)
{
    g_cfg = cfg;
}

const AnimTextConfig& anim_text_get_config()
{
    return g_cfg;
}

void anim_text_clear_cache()
{
    g_states.clear();
}

std::string anim_text_simple(const std::string& ru, const std::string& en, const char* id)
{
    // Твой язык:
    const bool is_russian = (var->lang.lang == 0);
    const std::string& target = is_russian ? ru : en;

    // IMPORTANT:
    // ID должен быть стабильным для конкретного "места" в UI, иначе не будет нормального перехода.
    // Если id == nullptr, внутри будет fallback на target (работает, но хуже/менее предсказуемо).
    return anim_text_target_internal(target, id);
}
