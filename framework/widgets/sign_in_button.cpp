#include "../headers/includes.h"
#include "../headers/widgets.h"

struct sign_in_button_state
{
    float alpha = 0.f;
    float progress = 0.f;
};

bool c_widgets::sign_in_button(std::string name, bool is_loading, float progress, float width)
{
    c_window* window = gui->get_window();
    c_id id = window->GetID(name.data());
    sign_in_button_state* state = gui->anim_container<sign_in_button_state>(id);

    c_vec2 pos = window->DC.CursorPos;
    float btn_w = width > 0.f ? width : 345.f;
    c_vec2 size = SCALE(btn_w, 40);
    c_vec2 line_size = SCALE(btn_w - 32.f, 8);
    c_vec2 inner_line_size = SCALE(btn_w - 34.f, 6);

    c_rect rect = { pos, pos + size };

    gui->item_add(rect, id);
    gui->item_size(rect);

    bool pressed = gui->button_behavior(rect, id, nullptr, nullptr);

    gui->easing(state->alpha, is_loading ? 1.f : 0.f, 6.f, dynamic_easing);
    if (is_loading)
    {
        if (progress > 0.f)
            gui->easing(state->progress, progress, 8.f, dynamic_easing);
        else
            gui->easing(state->progress, 1.f, 0.5f, static_easing);
    }
    else
    {
        state->progress = 0.f;
    }

    draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max,
        draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent), SCALE(8));

    draw->push_clip_rect(window->DrawList, rect.Min, rect.Max, true);

    if (state->alpha > 0.02f)
    {
        c_rect line_rect = {
            rect.GetCenter() - line_size / 2 + c_vec2{ 0, rect.GetHeight() * (1.f - state->alpha) },
            rect.GetCenter() + line_size / 2 + c_vec2{ 0, rect.GetHeight() * (1.f - state->alpha) }
        };

        draw->rect_filled(window->DrawList, line_rect.Min, line_rect.Max,
            draw->get_clr(clr->layout), SCALE(999));

        float fill_w = inner_line_size.x * ImClamp(state->progress, 0.05f, 1.0f);
        draw->rect_filled_multi_color(window->DrawList, line_rect.Min + SCALE(1, 1), 
            line_rect.Min + SCALE(1, 1) + c_vec2{ fill_w, inner_line_size.y },
            draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent), SCALE(999));
    }
    
    if (state->alpha < 0.98f)
    {
        draw->text_clipped(window->DrawList, font->get(inter_extra, 12), 
            rect.Min - c_vec2{ 0, rect.GetHeight() * state->alpha }, rect.Max - c_vec2{ 0, rect.GetHeight() * state->alpha }, 
            draw->get_clr(clr->white, 1.f - state->alpha), name.data(), 0, 0, { 0.5, 0.5 });
    }

    draw->pop_clip_rect(window->DrawList);

    return pressed && !is_loading;
}
