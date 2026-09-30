#pragma once
#include <string>

// Настройки эффекта
struct AnimTextConfig
{
    float duration      = 0.35f; // длительность полного перехода (сек). 0 = мгновенно
    float scramble_fps  = 30.0f; // как часто обновлять "шум" (раз в секунду). 0 = шум статичный

    bool  keep_spaces   = true;  // пробелы не превращаем в шум (держит читабельность)
    float gc_forget_after = 30.0f; // через сколько секунд без использования выкидывать состояние
    float gc_interval     = 5.0f;  // как часто делать GC (сек)
};

// Глобальные настройки/сервис
void anim_text_set_config(const AnimTextConfig& cfg);
const AnimTextConfig& anim_text_get_config();
void anim_text_clear_cache();

// Основная функция: выбирает RU/EN по var->lang.lang и возвращает строку для текущего кадра.
std::string anim_text_simple(const std::string& ru, const std::string& en, const char* id = nullptr);

// Макросы для стабильного уникального ID.
// __FUNCTION__ плох тем, что одинаковый внутри одной функции для разных строк.
// __FILE__+__LINE__ даёт “почти всегда” уникально.
#define ANIM_TEXT_STR2(x) #x
#define ANIM_TEXT_STR(x)  ANIM_TEXT_STR2(x)

#define ANIM_TEXT(ru, en)     anim_text_simple((ru), (en), __FILE__ ":" ANIM_TEXT_STR(__LINE__))
#define ANIM_TEXT_ID(ru, en, id) anim_text_simple((ru), (en), (id))
