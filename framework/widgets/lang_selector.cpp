#include "../headers/includes.h"

struct lang_selector_state_t
{
    bool open;
    float height = 57;
    float alpha;
};

void c_widgets::lang_selector(float* height)
{
    c_window* window = gui->get_window();
    if (window->SkipItems)
    {
        return;
    }

    std::string name = "lang_selector";
    c_id id = window->GetID(name.data());
    lang_selector_state_t* state = gui->anim_container<lang_selector_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = SCALE(30, 0) + c_vec2(0, state->height);
    c_rect rect = { pos, pos + size };

    c_rect button = { rect.Min, rect.Min + SCALE(30, 12) };

    gui->item_add(rect, id);
    gui->item_size(rect);

    bool pressed = gui->button_behavior(button, id, nullptr, nullptr);

    if (pressed)
    {
        state->open = !state->open;
    }

    gui->easing(state->alpha, state->open ? 1.f : 0.f, 8.f, dynamic_easing);
    gui->easing(state->height, state->open ? SCALE(82) : SCALE(57), 8.f, dynamic_easing);
    *height = state->height;

    draw->rotate_start(window->DrawList);

    draw->text_clipped(window->DrawList, font->get(icon_font, 12),
        button.Min, button.Max, draw->get_clr(clr->white), "A", 0, 0, { 0.5, 0.5 });

    draw->rotate_end(window->DrawList, 90.f + 180.f * state->alpha, button.GetCenter());

    c_rect lang_rect = { button.GetBL() + SCALE(0, 15), rect.Max };

    draw->rect_filled(window->DrawList, lang_rect.Min, lang_rect.Max, draw->get_clr(clr->lang_color), SCALE(999));

    draw->push_clip_rect(window->DrawList, lang_rect.Min, lang_rect.Max, true);

    c_rect active_flag_rect = { 
        lang_rect.GetBL() - SCALE(-5, 25), lang_rect.Max - SCALE(5, 5)
    };

    c_rect second_flag_rect = { 
        active_flag_rect.GetTL() - SCALE(0, 25), active_flag_rect.GetTR() - SCALE(0, 5)
    };

    if (second_flag_rect.Contains(gui->mouse_pos()) && gui->is_window_hovered(0) && gui->mouse_clicked(0))
    {
        if (var->lang.lang == 0)
        {
            var->lang.lang = 1;
        }
        else
        {
            var->lang.lang = 0;
        }
        state->open = false;
    }

    draw->image_rounded(window->DrawList, var->lang.lang == 0 ? var->img.ru_flag : var->img.us_flag, active_flag_rect.Min, active_flag_rect.Max,
        { 0, 0 }, { 1, 1 }, draw->get_clr(clr->white), SCALE(999));

    draw->image_rounded(window->DrawList, var->lang.lang == 1 ? var->img.ru_flag : var->img.us_flag, second_flag_rect.Min, second_flag_rect.Max,
        { 0, 0 }, { 1, 1 }, draw->get_clr(clr->white, state->alpha), SCALE(999));

    draw->pop_clip_rect(window->DrawList);
}