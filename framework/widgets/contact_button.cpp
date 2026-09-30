#include "../headers/includes.h"

struct contact_button_state_t
{
    c_vec4 color;
};

bool c_widgets::contact_button(std::string name, std::string icon)
{
    c_window* window = gui->get_window();
    if (window->SkipItems)
    {
        return false;
    }

    c_id id = window->GetID(name.data());

    contact_button_state_t* state = gui->anim_container<contact_button_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = SCALE(13, 13);
    c_rect rect = { pos, pos + size };

    gui->item_add(rect, id);
    gui->item_size(rect);

    bool hover;
    bool pressed = gui->button_behavior(rect, id, &hover, nullptr);

    gui->easing(state->color, hover ? clr->white.Value : clr->text_inactive.Value, 24.f, dynamic_easing);

    draw->text_clipped(window->DrawList, font->get(icon_font, 13),
        rect.Min, rect.Max, draw->get_clr(state->color),
        icon.data(), 0, 0, { 0.5, 0.5 });

    return pressed;
}