#include "../headers/includes.h"

struct popup_state_t
{
    float alpha;
    float btn_alpha;
    c_vec4 link;
};

bool c_widgets::popup(std::string id_text, bool* show, std::string icon, std::string name, std::string desc, std::string desc2, std::string button_text)
{
    c_window* window = gui->get_window();
    if (window->SkipItems)
    {
        return false;
    }

    c_id id = window->GetID(id_text.data());

    popup_state_t* state = gui->anim_container<popup_state_t>(id);

    gui->easing(state->alpha, *show ? 1.f : 0.f, 6.f, dynamic_easing);
    if (state->alpha < 0.02f)
    {
        return false;
    }

    bool value_changed = false;

    gui->set_pos({ 0, 0 }, pos_all);
    gui->push_var(style_var_alpha, state->alpha);
    gui->begin_content("Popup", SCALE(elements->window.size), { 0, 0 }, { 0, 0 }, window_flags_no_scroll_with_mouse | window_flags_no_scrollbar);
    {
        window = gui->get_window();
        c_rect rect = window->Rect();
        draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->layout, 0.6f),
            SCALE(elements->window.rounding));

        gui->dummy({ rect.GetWidth(), rect.GetHeight() * (1.f - state->alpha) + SCALE(80) });

        gui->set_pos(SCALE(227), pos_x);
        gui->begin_content("Inner", SCALE(330, 340), SCALE(5, 5), SCALE(0, 0), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
        {
            window = gui->get_window();
            c_rect rect = window->Rect();

            if (state->alpha > 0.99f)
            {
                if (!rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(0))
                {
                    *show = false;
                }
            }

            draw->rect_filled_multi_color(window->DrawList, rect.Min, rect.Max,
                draw->get_clr(clr->accent), draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), SCALE(12));
            draw->rect_filled(window->DrawList, rect.Min + SCALE(5, 5), rect.Max - SCALE(5, 5), draw->get_clr(clr->layout),
                SCALE(7));

            gui->begin_content("Header", SCALE(320, 223), SCALE(0, 50), SCALE(0, 20), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
            {
                gui->dummy({ gui->content_avail().x, SCALE(50) });
                c_rect rect = { GetItemRectMin(), GetItemRectMax() };

                draw->circle_filled(window->DrawList, rect.GetCenter(), SCALE(25), draw->get_clr(clr->accent));
                draw->text_clipped(window->DrawList, font->get(icon_font, 16), rect.Min, rect.Max, draw->get_clr(clr->black),
                    icon.data(), 0, 0, { 0.5, 0.5 });

                gui->dummy({ gui->content_avail().x, SCALE(11) });
                rect = { GetItemRectMin(), GetItemRectMax() };

                draw->text_clipped(window->DrawList, font->get(inter, 17),
                    rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10),
                    draw->get_clr(clr->white), name.data(), 0, 0, { 0.5, 0.5 });

                gui->dummy({ gui->content_avail().x, SCALE(22) });
                rect = { GetItemRectMin(), GetItemRectMax() };

                std::string first_line, second_line;
                bool second_is_link = false;

                size_t newline_pos = desc.find('\n');
                if (newline_pos != std::string::npos) {
                    first_line = desc.substr(0, newline_pos);
                    second_line = desc.substr(newline_pos + 1);
                    
                    second_is_link = second_line.find('(') != std::string::npos && 
                                    second_line.find(')') != std::string::npos &&
                                    second_line.find('[') != std::string::npos &&
                                    second_line.find(']') != std::string::npos;
                } else {
                    first_line = desc;
                }

                draw->text_clipped(window->DrawList, font->get(inter, 12),
                    rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10),
                    draw->get_clr(clr->text_inactive), first_line.data(), 0, 0, { 0.5, 0.25 });

                if (!second_line.empty()) {
                    auto color = second_is_link ? draw->get_clr(clr->accent) : draw->get_clr(clr->text_inactive);
                    
                    if (second_is_link) {
                        size_t start = second_line.find('(') + 1;
                        size_t end = second_line.find(')');
                        std::string link_text = second_line.substr(start, end - start);
                        
                        draw->text_clipped(window->DrawList, font->get(inter, 12),
                            rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10),
                            draw->get_clr(state->link), link_text.data(), 0, 0, { 0.5, 0.75 });
                        
                        c_vec2 text_size = gui->text_size(font->get(inter, 12), link_text.data());
                        c_vec2 current_pos = rect.GetCenter() - c_vec2{text_size.x / 2, -SCALE(15)};
                        float amount = text_size.x;
                        float amount2 = amount / 3;
                        for (int i = 0; i < amount2; ++i)
                        {
                            draw->circle_filled(window->DrawList,
                                current_pos, 1, draw->get_clr(state->link));
                            current_pos.x += text_size.x / amount2;
                        }

                        c_rect rect2 = { rect.GetCenter() - text_size / 2, rect.GetCenter() + text_size / 2 };
                        bool rect2_hov = rect2.Contains(gui->mouse_pos());
                        gui->easing(state->link, rect2_hov ? clr->white.Value : clr->accent.Value, 24.f, dynamic_easing);

                    } else {
                        draw->text_clipped(window->DrawList, font->get(inter, 12),
                            rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10),
                            color, second_line.data(), 0, 0, { 0.5, 0.75 });
                    }
                }

                c_vec2 current_pos = gui->get_window()->Rect().GetBL() - c_vec2{ 0, 1 };
                float amount = 61;
                float width = gui->get_window()->Rect().GetWidth() / amount;
                for (int i = 0; i < amount / 2; ++i)
                {
                    draw->line(window->DrawList, current_pos, current_pos + c_vec2{ width, 0 }, draw->get_clr(clr->child));
                    current_pos += c_vec2{ width * 2, 0 };
                }
            }
            gui->end_content();

            gui->begin_content("Bottom", SCALE(320, 107), SCALE(20, 20), SCALE(0, 20));
            {
                gui->dummy({ gui->content_avail().x, SCALE(7) });
                c_rect rect = { GetItemRectMin(), GetItemRectMax() };
                draw->text_clipped(window->DrawList, font->get(inter, 12),
                    rect.Min - SCALE(0, 10), rect.Max + SCALE(0, 10),
                    draw->get_clr(clr->text_inactive), desc2.data(), 0, 0, { 0.5, 0.5 });

                gui->dummy({ gui->content_avail().x, SCALE(40) }); 
                rect = { GetItemRectMin(), GetItemRectMax() };

                bool btn_hover = rect.Contains(gui->mouse_pos());
                gui->easing(state->btn_alpha, btn_hover ? 1.f : 0.f, 6.f, dynamic_easing);

                draw->rect_filled_multi_color(window->DrawList, rect.Min + SCALE(3, 2) * state->btn_alpha, rect.Max - SCALE(3, 2) * state->btn_alpha,
                    draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent), SCALE(8));

                draw->text_clipped(window->DrawList, font->get(inter, 12),
                    rect.Min, rect.Max,
                    draw->get_clr(clr->white), button_text.data(), 0, 0, { 0.5, 0.5 });
            
                value_changed = gui->button_behavior(rect, window->GetID(button_text.data()), nullptr, nullptr);
            }
            gui->end_content();
        }
        gui->end_content();
    }
    gui->end_content();
    gui->pop_var();

    return value_changed;
}