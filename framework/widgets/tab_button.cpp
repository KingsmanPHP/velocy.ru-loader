#include "../headers/includes.h"

struct tab_button_state_t
{
    float alpha;
    c_vec4 icon_clr;
};

bool c_widgets::begin_tab_button(std::string name, std::string icon, int tab, int buttons)
{
    c_window* window = gui->get_window();
    c_id id = window->GetID(name.data());
    tab_button_state_t* state = gui->anim_container<tab_button_state_t>(id);

    bool open = tab == var->tab.stored;

    gui->easing(state->alpha, open ? 1.f : 0.f, 6.f, dynamic_easing);
    gui->easing(state->icon_clr, open ? clr->accent.Value : clr->text_inactive.Value, 6.f, dynamic_easing);

    float height = (SCALE(30) * buttons) * state->alpha;
    gui->begin_content(name + "Outline", SCALE(30, 30) + c_vec2{ 0, height }, { 0, 0 }, { 0, 0 }, window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
    
    c_rect rect = gui->get_window()->Rect();

    if (rect.Contains(gui->mouse_pos()) && gui->is_window_hovered(0) && gui->mouse_clicked(0))
    {
        var->tab.stored = tab;
    }

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->accent, 0.1f * state->alpha), SCALE(999));
    draw->text_clipped(window->DrawList, font->get(icon_font, 12),
        rect.Min, rect.Min + SCALE(30, 30), draw->get_clr(state->icon_clr), icon.data(), 0, 0, { 0.5, 0.5 });

    gui->push_var(style_var_alpha, state->alpha);
    gui->dummy(SCALE(30, 29 * state->alpha));
    gui->button_behavior(rect, id, nullptr, nullptr);

    return state->alpha > 0;
}

void c_widgets::end_tab_button()
{
    gui->end_content();
    gui->pop_var();
    gui->dummy(SCALE(0, 10));
}