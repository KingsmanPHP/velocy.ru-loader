#include "../headers/includes.h"

struct product_button_state_t {
    c_vec4 layout_clr = clr->layout.Value;
    c_vec4 icon_clr = clr->white.Value;
    float alpha = 0.f;
};

bool c_widgets::product_button(std::string id_text, std::string name, std::string desc, std::string flag, std::string flag_icon, ImTextureID person, float width, bool locked)
{
    c_window* window = gui->get_window();
    if (window->SkipItems)
    {
        return false;
    }

    c_id id = window->GetID(id_text.data());

    product_button_state_t* state = gui->anim_container<product_button_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = SCALE(347, 185);
    c_rect rect = { pos, pos + size };
    c_rect inner = { rect.Min + SCALE(15, 15), rect.Max - SCALE(15, 15) };

    gui->item_add(rect, id);
    gui->item_size(rect);

    draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.3f), draw->get_clr(clr->accent, 0.3f), draw->get_clr(clr->accent), SCALE(9));

    draw->image_rounded(window->DrawList, person, rect.Min, rect.Min + SCALE(0, 185) + c_vec2{ width, 0 }, { 0, 0 }, { 1, 1 }, draw->get_clr(clr->white), SCALE(8));

    draw->image_rounded(window->DrawList, var->img.product_layout, rect.Min, rect.Max, { 0, 0 }, { 1, 1 }, draw->get_clr(clr->white), SCALE(9));

    draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->accent, 0.2f), draw->get_clr(clr->accent, 0.06f), draw->get_clr(clr->accent, 0.06f), draw->get_clr(clr->accent, 0.2f), SCALE(9));

    c_rect icon_rect = { inner.GetTR() - SCALE(26, 0), inner.GetTR() + SCALE(0, 26) };

    c_vec2 flag_size = gui->text_size(font->get(inter, 10), flag.data());
    c_rect flag_rect = { icon_rect.GetTL() - SCALE(25, 0) - c_vec2{ flag_size.x, 0 }, icon_rect.GetBL() - SCALE(5, 0) };
    
    draw->rect_filled(window->DrawList, icon_rect.Min, icon_rect.Max, draw->get_clr(clr->layout), SCALE(5));
    draw->rect_filled(window->DrawList, flag_rect.Min, flag_rect.Max, draw->get_clr(clr->layout), SCALE(5));

    draw->text_clipped(window->DrawList, font->get(icon_font, 10), icon_rect.Min, icon_rect.Max, draw->get_clr(clr->white),
        flag_icon.data(), 0, 0, { 0.5, 0.5 });

    draw->text_clipped(window->DrawList, font->get(inter, 10), flag_rect.Min, flag_rect.Max, draw->get_clr(clr->white),
        flag.data(), 0, 0, { 0.5, 0.5 });

    draw->text_clipped(window->DrawList, font->get(inter, 10),
        inner.Min, inner.Max, draw->get_clr(clr->white), desc.data(), 0, 0, { 0, 1 });

    c_vec2 desc_size = gui->text_size(font->get(inter, 10), desc.data());

    c_rect name_rect = { inner.GetBL() - c_vec2{ 0, desc_size.y } - SCALE(0, 32), inner.GetBL() - c_vec2{ -desc_size.x, desc_size.y } - SCALE(0, 15) };

    draw->image(window->DrawList, var->img.umbrella_logo,
        name_rect.Min, name_rect.Min + SCALE(18, 18), { 0, 0 }, { 1, 1 }, draw->get_clr(clr->white));

    draw->text_clipped(window->DrawList, font->get(inter_extra, 17), name_rect.Min + SCALE(33, 0), name_rect.Max,
        draw->get_clr(clr->white), name.data(), 0, 0, { 0, 0.5 });

    c_rect run_rect = { inner.GetBR() - SCALE(35, 35) - SCALE(3, 3) * state->alpha, inner.GetBR() + SCALE(3, 3) * state->alpha };

    draw->rect_filled(window->DrawList, run_rect.Min, run_rect.Max, 
        locked ? draw->get_clr(clr->layout, 0.6f) : draw->get_clr(state->layout_clr), SCALE(5));

    const char* run_icon = locked ? "J" : "L";
    draw->text_clipped(window->DrawList, font->get(icon_font, 12), run_rect.Min, run_rect.Max,
        locked ? draw->get_clr(clr->text_inactive) : draw->get_clr(state->icon_clr), run_icon, 0, 0, { 0.5, 0.5 });

    if (locked)
    {
        return false;
    }

    bool hover;
    bool pressed = gui->button_behavior(run_rect, id, &hover, nullptr);

    gui->easing(state->layout_clr, hover ? clr->accent.Value : clr->layout.Value, 6.f, dynamic_easing);
    gui->easing(state->icon_clr, hover ? clr->black.Value : clr->white.Value, 6.f, dynamic_easing);
    gui->easing(state->alpha, hover ? 1.f : 0.f, 6.f, dynamic_easing);
    
    return pressed;
}