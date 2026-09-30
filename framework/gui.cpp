#include "headers/includes.h"
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <shellapi.h>
#include "helpers/velocy_auth.h"
#include "helpers/product_manager.h"
#include "helpers/updater.h"
#include "modules/fivem_module.h"
#include "modules/fortnite_module.h"

static char g_auth_email[128] = "";
static char g_auth_password[128] = "";
static bool g_show_pwd = false;
static std::atomic<bool> g_is_logging_in{ false };
static std::atomic<bool> g_is_browser_logging_in{ false };
static float g_login_progress = 0.f;
static std::string g_auth_error = "";
static bool g_show_auth_error_popup = false;
static bool g_session_initialized = false;

void move_window(HWND hwnd, RECT rc)
{
	c_rect rect(c_vec2{ 0, 0 }, SCALE(elements->titlebar.size));
	bool hovered = gui->is_window_hovered(0) && rect.Contains(gui->mouse_pos());
	static bool clicked = false;

	if (hovered && GetIO().MouseClicked[0])
		clicked = true;

	if (!GetIO().MouseDown[0])
		clicked = false;

	c_vec2 size = SCALE(elements->window.size);

	if (clicked)
	{
		GetWindowRect(hwnd, &rc);
		MoveWindow(hwnd, rc.left + GetMouseDragDelta().x, rc.top + GetMouseDragDelta().y, size.x, size.y, FALSE);
	}

	while (rc.right - rc.left != size.x || rc.bottom - rc.top != size.y)
	{
		GetWindowRect(hwnd, &rc);
		MoveWindow(hwnd, rc.left, rc.top, size.x, size.y, FALSE);
	}
}

std::string to_utf8(const wchar_t* wstr) {
    if (!wstr) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], size_needed, NULL, NULL);
    
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

void c_gui::render()
{
	static bool is_account_disabled = false;
	static bool is_account_disabled_block = false;

	static bool v1_inject = false;
	static float v1_alpha = 0.f;
	static bool v1_popup_show = false;

	static bool v2_inject = false;
	static float v2_alpha = 0.f;
	static bool v2_popup_show = false;

	static bool v3_inject = false;
	static float v3_alpha = 0.f;
	static bool v3_popup_show = false;

	static bool v4_inject = false;
	static float v4_alpha = 0.f;
	static bool v4_popup_show = false;

	static bool v5_inject = false;
	static float v5_alpha = 0.f;
	static bool v5_popup_show = false;

	static bool v6_inject = false;
	static float v6_alpha = 0.f;
	static bool v6_popup_show = false;

	if (ImGui::IsKeyPressed(ImGuiKey_F1))
	{
		var->gui.stored_dpi += 10;
		var->gui.dpi_changed = true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_F2))
	{
		var->gui.stored_dpi -= 10;
		var->gui.dpi_changed = true;
	}

	if (!g_session_initialized)
	{
		g_session_initialized = true;
		AutoUpdater::CheckForUpdatesAsync();
		if (VelocyAuth::CarregarSessao() && VelocyAuth::GetSession().authenticated)
		{
			var->tab.active = var->tab.stored = 1;
		}
	}

	if (!VelocyAuth::GetSession().authenticated)
	{
		if (var->tab.active != 0)
			var->tab.active = 0;
		if (var->tab.stored != 0)
			var->tab.stored = 0;
	}
	else
	{
		if (var->tab.active != 1)
			var->tab.active = 1;
		if (var->tab.stored != 1)
			var->tab.stored = 1;
	}

	gui->easing(var->tab.alpha, var->tab.active != var->tab.stored ? 0.f : 1.f, 4.f, static_easing);
	if (var->tab.alpha == 0)
	{
		var->tab.active = var->tab.stored;
	}

	gui->initialize();
	gui->set_next_window_size(SCALE(elements->window.size));
	gui->set_next_window_pos({ 0, 0 });
	gui->begin(elements->window.name, nullptr, window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_bring_to_front_on_focus | window_flags_no_focus_on_appearing | window_flags_no_background | window_flags_no_decoration);
	{
		gui->set_style();
		gui->draw_decorations();

		gui->begin_content("Titlebar", SCALE(elements->titlebar.size), { 0, 0 }, { 0, 0 });
		{
			c_window* window = gui->get_window();
			c_rect rect = window->Rect();

			draw->image(window->DrawList, var->img.logo, rect.Min + SCALE(19, 19), rect.Min + SCALE(19, 19) + SCALE(22, 22), { 0, 0 }, { 1, 1 }, draw->get_clr(ImColor{ 255, 255, 255 }));

			draw->text_clipped(window->DrawList, font->get(inter, 14), rect.Min + SCALE(60, 0), rect.Max, draw->get_clr(clr->white), "velocy.ru", 0, 0, { 0, 0.5 });

			std::string user_display = VelocyAuth::GetSession().authenticated ? 
				VelocyAuth::GetSession().username : 
				(var->lang.lang == 0 ? "Гость" : "Guest");

			draw->text_clipped(window->DrawList, font->get(inter, 14), rect.Min, rect.Max - SCALE(60, 0), draw->get_clr(clr->white), user_display.data(), 0, 0, { 1, 0.5 });

			bool logout_hovered = false;
			if (VelocyAuth::GetSession().authenticated)
			{
				c_vec2 u_size = gui->text_size(font->get(inter, 14), user_display.data());
				c_vec2 btn_center = { rect.Max.x - SCALE(60) - u_size.x - SCALE(18), rect.GetCenter().y };
				c_rect logout_rect = { btn_center - SCALE(10, 10), btn_center + SCALE(10, 10) };
				logout_hovered = logout_rect.Contains(gui->mouse_pos());
				static float logout_alpha = 0.f;
				gui->easing(logout_alpha, logout_hovered ? 1.f : 0.f, 16.f, dynamic_easing);

				draw->circle_filled(window->DrawList, btn_center, SCALE(9), draw->get_clr(clr->child, 0.5f + 0.4f * logout_alpha));
				draw->text_clipped(window->DrawList, font->get(icon_font, 11), logout_rect.Min, logout_rect.Max,
					logout_hovered ? draw->get_clr(clr->accent) : draw->get_clr(clr->text_inactive), "K", 0, 0, { 0.5, 0.5 });

				if (logout_hovered && gui->mouse_clicked(0))
				{
					VelocyAuth::LimparSessao();
					var->tab.stored = 0;
					var->tab.active = 0;
				}
			}

			c_rect avatar_rect = { rect.GetBR() - SCALE(45, 45), rect.GetBR() - SCALE(15, 15) };

			draw->circle_filled(window->DrawList, avatar_rect.GetCenter(), avatar_rect.GetHeight() / 2, draw->get_clr(clr->accent));
			draw->image(window->DrawList, var->img.owl_logo, avatar_rect.GetCenter() - SCALE(12, 11), avatar_rect.GetCenter() + SCALE(12, 14), { 0, 0 }, { 1, 1 }, draw->get_clr(clr->black));
		
			if (!logout_hovered)
				move_window(var->winapi.hwnd, var->winapi.rc);
		}
		gui->end_content();

		gui->begin_content("Content1", SCALE(elements->content1.size), { 0, 0 }, { 0, 0 });
		{
			c_window* window = gui->get_window();
			c_rect rect = window->Rect();

			c_rect tab_icon_rect = { rect.Min + SCALE(15, 0), rect.GetBL() + SCALE(45, 0) };

			std::string icon = var->tab.active == 0 ? "H" : var->tab.active == 1 ? "M" : "N";

			std::string text1 = 
				var->tab.active == 0 ? anim_text_simple("Главная", "Main", "tab_text1_active0") :
				var->tab.active == 1 ? anim_text_simple("Продукт", "Product", "tab_text1_active1") :
				anim_text_simple("Пусто", "Empty", "tab_text1_other");

			std::string text2 = 
				var->tab.active == 0 ? anim_text_simple("Авторизация пользователя", "User authorization", "tab_text2_active0") :
				var->tab.active == 1 ? anim_text_simple("Продукты", "Products", "tab_text2_active1") :
				anim_text_simple("Пусто", "Empty", "tab_text2_other");

			gui->push_var(style_var_alpha, var->tab.alpha);

			draw->text_clipped(window->DrawList, font->get(icon_font, 12), 
				tab_icon_rect.Min, tab_icon_rect.Max, draw->get_clr(clr->text_inactive), icon.data(), 0, 0, { 0.5, 0.5 });

			draw->text_clipped(window->DrawList, font->get(inter, 14), rect.Min + SCALE(60, 0), rect.Max,
				draw->get_clr(clr->text_inactive), text1.data(), 0, 0, { 0, 0.5 });

			c_vec2 text_size = gui->text_size(font->get(inter, 14), text1.data());
			c_vec2 circle_center = { rect.Min.x + SCALE(70) + text_size.x, rect.GetCenter().y };

			draw->circle_filled(window->DrawList, circle_center, SCALE(2), draw->get_clr(clr->text_inactive));

			draw->text_clipped(window->DrawList, font->get(inter, 14), { circle_center.x + SCALE(10), rect.Min.y },
				rect.Max, draw->get_clr(clr->text_inactive), text2.data(), 0, 0, { 0, 0.5 });

			gui->pop_var();

			gui->set_pos(SCALE(701, 9), pos_all);
			widgets->contact_button("Discord", "F");
			gui->sameline(0, SCALE(15));
			widgets->contact_button("Telegram", "P");
			gui->sameline(0, SCALE(15));
			widgets->contact_button("Youtube", "R");
			if (VelocyAuth::GetSession().authenticated)
			{
				gui->sameline(0, SCALE(15));
				if (widgets->contact_button("Logout", "K"))
				{
					VelocyAuth::LimparSessao();
					var->tab.stored = 0;
					var->tab.active = 0;
				}
			}
		}
		gui->end_content();

		gui->begin_content("Content2", SCALE(elements->content2.size), SCALE(elements->content2.padding), SCALE(elements->content2.padding), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
		{
			c_window* window = gui->get_window();

			c_vec2 pos;
			c_vec2 size;

			gui->begin_content("Tab bar", SCALE(elements->tab_bar.size), { 0, 0 }, { 0, 0 });
			{
				if (VelocyAuth::GetSession().authenticated)
				{
					widgets->begin_tab_button("Product", "M", 1, 1);
					{
						gui->dummy(SCALE(30, 30));
						{
							c_rect rect = { GetItemRectMin(), GetItemRectMax() };
							draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->accent), SCALE(999));
							draw->text_clipped(gui->window_drawlist(), font->get(icon_font, 12),
								rect.Min, rect.Max, draw->get_clr(clr->black), "G", 0, 0, { 0.5, 0.5 });
						}
					}
					widgets->end_tab_button();
				}

				static float height;

				gui->set_pos({ 0, gui->get_window()->Rect().GetHeight() - height }, pos_all);

				widgets->lang_selector(&height);

				pos = gui->get_window()->Pos;
				size = gui->get_window()->Size;
			}
			gui->end_content();

			if (is_account_disabled_block)
			{
				gui->set_pos(pos - gui->get_window()->Pos, pos_all);
				gui->begin_content("Block", size);
				{

				}
				gui->end_content();
			}

			gui->sameline();

			gui->push_var(style_var_alpha, var->tab.alpha);

			if (var->tab.active == 0 && !VelocyAuth::GetSession().authenticated)
			{
				gui->begin_content("Login form", SCALE(elements->login_form.size), SCALE(0, 0), SCALE(0, 8));
				{
					c_rect rect = gui->get_window()->Rect();

					std::string first_text = anim_text_simple("Испытайте", "Experience", "experience_text");
					std::string second_text = anim_text_simple("безумие", "the madness", "madness_text");

					ImFont* text_font1 = var->lang.lang == 0 ? font->get(start2p, 41) : font->get(jersey, 88);
					ImFont* text_font2 = var->lang.lang == 0 ? font->get(start2p, 52) : font->get(jersey, 75);

					struct letter_t {
						std::string letter;
						float alpha;
						float alpha_stored;
					};

					std::vector<letter_t> text1_letters;
					for (size_t i = 0; i < first_text.size();) {
						unsigned char c = first_text[i];
						
						if ((c == 0xD0 || c == 0xD1) && i + 1 < first_text.size()) {
							std::string letter = first_text.substr(i, 2);
							text1_letters.push_back({ letter, 1.0f, get_random_float() });
							i += 2;
						} else {
							std::string letter(1, c);
							text1_letters.push_back({ letter, 1.0f, get_random_float() });
							i += 1;
						}
					}

					std::vector<letter_t> text2_letters;
					for (size_t i = 0; i < second_text.size();) {
						unsigned char c = second_text[i];
						
						if ((c == 0xD0 || c == 0xD1) && i + 1 < second_text.size()) {
							std::string letter = second_text.substr(i, 2);
							text2_letters.push_back({ letter, 1.0f, get_random_float() });
							i += 2;
						} else {
							std::string letter(1, c);
							text2_letters.push_back({ letter, 1.0f, get_random_float() });
							i += 1;
						}
					}

					ImColor accent_color = draw->get_clr(clr->accent);
					ImColor white_color = draw->get_clr(clr->white);

					static float time_counter = 0.0f;
					time_counter += ImGui::GetIO().DeltaTime;

					c_vec2 current_pos = rect.Min - SCALE(0, var->lang.lang == 0 ? 0 : 15);
					for (size_t i = 0; i < text1_letters.size(); i++) {
						auto& letter = text1_letters[i];

						float phase = i * 0.3f;
						float sin_val = std::sin(time_counter * 3.0f + phase);
						float target = std::pow((sin_val + 1.0f) * 0.5f, 2.0f);
						letter.alpha = letter.alpha + (target - letter.alpha) * 0.4f;
											
						c_vec2 letter_size = gui->text_size(text_font1, letter.letter.data());
						
						draw->text_clipped(window->DrawList, text_font1,
							current_pos, rect.Max, draw->get_clr(clr->accent, letter.alpha), letter.letter.data(), 0, 0, { 0, 0 });
						
						current_pos.x += letter_size.x;
					}

					current_pos = rect.Min + c_vec2{ 0, gui->text_size(text_font1, "E").y - SCALE(var->lang.lang == 0 ? -5 : 33) };
					for (size_t i = 0; i < text2_letters.size(); i++) {
						auto& letter = text2_letters[i];

						float phase = i * 0.3f;
						float sin_val = std::sin(time_counter * 3.0f + phase);
						float target = std::pow((sin_val + 1.0f) * 0.5f, 2.0f);
						letter.alpha = letter.alpha + (target - letter.alpha) * 0.4f;
						
						c_vec2 letter_size = gui->text_size(text_font2, letter.letter.data());
						
						draw->text_clipped(window->DrawList, text_font2,
							current_pos, rect.Max, draw->get_clr(clr->white, letter.alpha), letter.letter.data(), 0, 0, { 0, 0 });
						
						current_pos.x += letter_size.x;
					}
					float subtitle_y = var->lang.lang == 0 ? 105.f : 132.f;
					float dummy_h = var->lang.lang == 0 ? 135.f : 168.f;

					gui->dummy({ gui->content_avail().x, SCALE(dummy_h) });

					draw->text_clipped(window->DrawList, font->get(inter, 14), rect.Min + SCALE(0, subtitle_y), rect.Max,
						draw->get_clr(clr->text_inactive), 
						anim_text_simple("Авторизуйтесь для доступа ко всем продуктам", "Log in to continue your journey through our products", "login_text").data(), 
						0, 0, { 0, 0 });

					widgets->key_input(anim_text_simple("Введите почту или логин", "Insert your email or username", "email_text"), g_auth_email, sizeof(g_auth_email), false, nullptr);

					widgets->key_input(anim_text_simple("Введите пароль", "Insert your password", "pass_text"), g_auth_password, sizeof(g_auth_password), true, &g_show_pwd);

					if (widgets->sign_in_button(anim_text_simple("ВОЙТИ", "SIGN IN", "sign_in_text"), g_is_logging_in.load(), g_login_progress))
					{
						if (!g_is_logging_in.load() && !g_is_browser_logging_in.load())
						{
							g_is_logging_in = true;
							g_login_progress = 0.2f;
							std::thread([]() {
								std::string err;
								bool ok = VelocyAuth::LoginDireto(g_auth_email, g_auth_password, err);
								g_is_logging_in = false;
								if (ok)
								{
									var->tab.stored = 1;
								}
								else
								{
									g_auth_error = err.empty() ? (var->lang.lang == 0 ? "Ошибка авторизации" : "Authentication failed") : err;
									g_show_auth_error_popup = true;
								}
							}).detach();
						}
					}

					gui->dummy(gui->content_avail());
					{
						c_rect dm_rect = { GetItemRectMin(), GetItemRectMax() };

						draw->text_clipped(window->DrawList, font->get(inter, 14), dm_rect.GetBL() - SCALE(0, 30), dm_rect.Max,
							draw->get_clr(clr->text_inactive), 
							anim_text_simple("velocy.ru @ 2026 Все права защищены", "velocy.ru @ 2026 All rights reserved", "velocy_loader_rights").data(), 
							0, 0, { 0, 0.5 });
					}
				}
				gui->end_content();

				gui->sameline();

				gui->begin_content("Login image", SCALE(elements->login_image.size), { 0, 0 }, { 0, 0 });
				{
					c_rect rect = gui->get_window()->Rect();

					draw->image(window->DrawList, var->img.content, rect.Min - SCALE(24, 0), rect.Max, { 0, 0 }, { 1, 1 },
						draw->get_clr(clr->white));
				}
				gui->end_content();
			}
			if (var->tab.active == 1)
			{
				gui->begin_content("Games", SCALE(710, 395), { 0, 0 }, SCALE(15, 15), window_flags_no_scrollbar);

				gui->begin_group();

				if (widgets->product_button(
					"fivem_prod",
					anim_text_simple("FIVEM EXTERNAL", "FIVEM EXTERNAL", "fivem_text"),
					anim_text_simple("FiveM External Overlay com Aimbot e ESP.\nOtimizado para desempenho competitivo,\nseguro e totalmente indetectável.", "FiveM External Overlay with Aimbot and ESP.\nOptimized for competitive performance,\nsafe and fully undetected.", "fivem_desc"),
					anim_text_simple("FIVEM", "FIVEM", "fivem_flag"),
					"O",
					var->img.fivem ? var->img.fivem : (var->img.product_layout ? var->img.product_layout : var->img.voids),
					SCALE(125)
				))
				{
					if (!VelocyAuth::GetSession().authenticated)
					{
						is_account_disabled = true;
					}
					else
					{
						v1_inject = true;
						v1_alpha = 0.f;
					}
				}

				gui->end_group();

				gui->sameline();

				gui->begin_group();

				if (widgets->product_button(
					"fortnite_prod",
					anim_text_simple("FORTNITE EXTERNAL", "FORTNITE EXTERNAL", "fn_text"),
					anim_text_simple("External indetectável com Aimbot suave e ESP\ncompleto. Produto em desenvolvimento e otimização\npara a temporada atual.", "Undetected External with smooth Aimbot and complete\nESP. In active development and optimization for the\ncurrent competitive season.", "fn_desc"),
					anim_text_simple("В РАЗРАБОТКЕ", "IN DEV", "fn_flag"),
					"E",
					var->img.fortnite ? var->img.fortnite : var->img.jg,
					SCALE(180),
					true
				))
				{
					// Produto bloqueado em manutenção
				}

				gui->end_group();

				gui->dummy({ gui->content_avail().x, SCALE(15) });

				c_rect window_rect = gui->get_window()->Rect();

				static float shadow_alpha1 = 0;
				static float shadow_alpha2 = 0;
				gui->easing(shadow_alpha1, gui->get_window()->Scroll.y > SCALE(5) ? 1.f : 0.f, 12.f, dynamic_easing);
				gui->easing(shadow_alpha2, gui->get_window()->Scroll.y < (gui->get_window()->ScrollMax.y - SCALE(15)) ? 1.f : 0.f, 12.f, dynamic_easing);
				
				draw->rect_filled_multi_color(gui->window_drawlist(),
					window_rect.Min, window_rect.GetTR() + SCALE(0, 200 * shadow_alpha1),
					draw->get_clr(clr->layout, shadow_alpha1), draw->get_clr(clr->layout, shadow_alpha1),  draw->get_clr(clr->layout, 0),  draw->get_clr(clr->layout, 0));
				
					draw->rect_filled_multi_color(gui->window_drawlist(),
					window_rect.GetBL() - SCALE(0, 200 * (shadow_alpha2)), window_rect.Max,
					draw->get_clr(clr->layout, 0), draw->get_clr(clr->layout, 0),  draw->get_clr(clr->layout, shadow_alpha2),  draw->get_clr(clr->layout, shadow_alpha2));
				
				gui->end_content();
			}

			gui->pop_var();
		}
		gui->end_content();

		widgets->popup("acc_disabled_popup", &is_account_disabled, 
			"K", 
			var->lang.lang == 0 ? "Аккаунт отключён" : "The account is disabled", 
			var->lang.lang == 0 ? "Доступ к аккаунту временно ограничен.\nВозможно, истёк срок действия лицензии." : "Account access is currently disabled. This\nmay be due to an expired license.", 
			var->lang.lang == 0 ? "Считаете, что это ошибка? Напишите в поддержку." : "Do you think this is a mistake? Contact support", 
			var->lang.lang == 0 ? "ПОДДЕРЖКА" : "SUPPORT"
		);

		widgets->popup("auth_error_popup", &g_show_auth_error_popup, 
			"D", 
			var->lang.lang == 0 ? "Ошибка авторизации" : "Authentication Failed", 
			g_auth_error.empty() ? (var->lang.lang == 0 ? "Проверьте введенные данные." : "Check your credentials.") : g_auth_error, 
			var->lang.lang == 0 ? "Нужна помощь? Свяжитесь с нами." : "Need help? Contact support.", 
			var->lang.lang == 0 ? "ПОДДЕРЖКА" : "SUPPORT"
		);

		if (v1_inject)
		{
			gui->easing(v1_alpha, 1.f, 0.25f, static_easing);
		}
		if (v1_alpha > 0.98f)
		{
			v1_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v1_alpha = 0.f;
				timer = 0.f;
				std::string launchErr;
				if (Loader::ProductManager::Instance().LaunchProduct("prod-fivem", launchErr))
				{
					v1_popup_show = true;
				}
				else
				{
					g_auth_error = launchErr.empty() ? "Erro ao inicializar Rocket Baimless." : launchErr;
					v3_popup_show = true;
				}
			}
		}
		widgets->inject("ambv1_inject", &v1_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v1_alpha);

		if (v2_inject)
		{
			gui->easing(v2_alpha, 1.f, 0.25f, static_easing);
		}
		if (v2_alpha > 0.98f)
		{
			v2_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v2_alpha = 0.f;
				timer = 0.f;
				v2_popup_show = true;
			}
		}
		widgets->inject("ambv2_inject", &v2_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v2_alpha);

		if (v3_inject)
		{
			gui->easing(v3_alpha, 1.f, 0.25f, static_easing);
		}
		if (v3_alpha > 0.98f)
		{
			v3_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v3_alpha = 0.f;
				timer = 0.f;
				v3_popup_show = true;
			}
		}
		widgets->inject("ambv3_inject", &v3_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v3_alpha);

		if (v4_inject)
		{
			gui->easing(v4_alpha, 1.f, 0.25f, static_easing);
		}
		if (v4_alpha > 0.98f)
		{
			v4_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v4_alpha = 0.f;
				timer = 0.f;
				v4_popup_show = true;
			}
		}
		widgets->inject("ambv4_inject", &v4_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v4_alpha);

		if (v5_inject)
		{
			gui->easing(v5_alpha, 1.f, 0.25f, static_easing);
		}
		if (v5_alpha > 0.98f)
		{
			v5_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v5_alpha = 0.f;
				timer = 0.f;
				v5_popup_show = true;
			}
		}
		widgets->inject("ambv5_inject", &v5_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v5_alpha);

		if (v6_inject)
		{
			gui->easing(v6_alpha, 1.f, 0.25f, static_easing);
		}
		if (v6_alpha > 0.98f)
		{
			v6_inject = false;
			static float timer = 0.f;
			timer += gui->fixed_speed(4.f);
			if (timer > 2.f)
			{
				v6_alpha = 0.f;
				timer = 0.f;
				v6_popup_show = true;
			}
		}
		widgets->inject("ambv6_inject", &v6_inject, 
			var->lang.lang == 0 ? "Загрузка всех необходимых данных" : "Loading all necessary data", v6_alpha);


		widgets->popup("ambv1_popup", &v1_popup_show, 
			"B", 
			var->lang.lang == 0 ? "Успешная сессия!" : "Successful session!", 
			var->lang.lang == 0 ? "Мы будем рады любому отзыву у нас на\n(Нашем сайте.)[https://discord.gg/DfgZB7G8HF]" : "We would be glad to receive your feedback on\n(Our website.)[https://discord.gg/DfgZB7G8HF]", 
			var->lang.lang == 0 ? "Имеете проблемы с инжектором?" : "Are you having a problem with your injector?", 
			var->lang.lang == 0 ? "СООБЩИТЬ ОБ ОШИБКЕ" : "REPORT BUGS"
		);
		
		if (widgets->popup("update_popup", &AutoUpdater::g_show_update_modal, 
			"B", 
			var->lang.lang == 0 ? "Доступно обновление!" : "Update Available!", 
			AutoUpdater::g_update_info.changelog.empty() ? 
				(var->lang.lang == 0 ? "Доступна новая версия лоудера." : "A new version of the loader is available.") : 
				AutoUpdater::g_update_info.changelog.c_str(), 
			var->lang.lang == 0 ? "Обновить сейчас до последней версии?" : "Update now to the latest version?", 
			AutoUpdater::g_is_updating.load() ? 
				(var->lang.lang == 0 ? "ОБНОВЛЕНИЕ..." : "UPDATING...") : 
				(var->lang.lang == 0 ? "ОБНОВИТЬ СЕЙЧАС" : "UPDATE NOW")
		))
		{
			if (!AutoUpdater::g_is_updating.load())
			{
				AutoUpdater::DownloadAndApplyUpdate(AutoUpdater::g_update_info.download_url);
			}
		}
	}
	gui->end();
}