#include "../headers/includes.h"

struct inject_state_t
{
    float alpha;
};

bool c_widgets::inject(std::string id_text, bool* show, std::string name, float alpha)
{
    c_window* window = gui->get_window();
    if (window->SkipItems)
    {
        return false;
    }

    c_id id = window->GetID(id_text.data());

    inject_state_t* state = gui->anim_container<inject_state_t>(id);

    gui->easing(state->alpha, *show ? 1.f : 0.f, 6.f, dynamic_easing);
    if (state->alpha < 0.02f)
    {
        return false;
    }
    if (alpha > 0.99f)
    {
        *show = false;
    }

    float vis_value = alpha * 100;
    char buf[64];
    gui->get_fmt(buf, &vis_value, "%.0f");

    gui->set_pos({ 0, 0 }, pos_all);
    gui->push_var(style_var_alpha, state->alpha);
    gui->begin_content("Popup", SCALE(elements->window.size), { 0, 0 }, { 0, 0 }, window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
    {
        window = gui->get_window();
        c_rect rect = window->Rect();
        draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->layout, 0.6f), SCALE(elements->window.rounding));

        gui->dummy({ rect.GetWidth(), rect.GetHeight() * (1.f - state->alpha) + SCALE(215) });

        gui->set_pos(SCALE(187), pos_x);
        gui->begin_content("Inner", SCALE(410, 70), SCALE(5, 5), SCALE(0, 0), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
        {
            window = gui->get_window();
            c_rect rect = window->Rect();

            if (alpha > 0.98f)
            {
                *show = false;
            }

            draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max,
                draw->get_clr(clr->accent), draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), SCALE(12));
            draw->rect_filled(window->DrawList, rect.Min + SCALE(5, 5), rect.Max - SCALE(5, 5), draw->get_clr(clr->layout),
                SCALE(7));

            gui->begin_content("Header", SCALE(400, 60), SCALE(15, 15), SCALE(15, 15), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
            {
                gui->dummy({ gui->content_avail().x, SCALE(7) });
                rect = { GetItemRectMin(), GetItemRectMax() };

                draw->text_clipped(window->DrawList, font->get(inter, 12),
                    rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10), draw->get_clr(clr->white),
                    name.data(), 0, 0, { 0, 0.5 }
                );

                std::string buf_str = buf;
                draw->text_clipped(window->DrawList, font->get(inter, 12),
                    rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10), draw->get_clr(clr->white),
                    std::string(buf_str + "%").data(), 0, 0, { 1, 0.5 }
                );

                gui->dummy({ gui->content_avail().x, SCALE(8) });
                rect = { GetItemRectMin(), GetItemRectMax() };

                draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->loading_color), SCALE(999));
                draw->rect_filled(window->DrawList, rect.Min + SCALE(1, 1), rect.Min + SCALE(1, 1) + c_vec2{( rect.GetWidth() - SCALE(2)) * alpha, SCALE(6) },
                    draw->get_clr(clr->accent), SCALE(999));
            }
            gui->end_content();
        }
        gui->end_content();
    }
    gui->end_content();
    gui->pop_var();

    return true;
}