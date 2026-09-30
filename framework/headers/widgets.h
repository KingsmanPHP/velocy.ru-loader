#pragma once
#include "includes.h"
#include "../headers/config.h"

#define IMGUI_DEFINE_MATH_OPERATORS

inline float get_random_float() {
    static unsigned int seed = 12345;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)seed / (float)0x7fffffff;
}

class c_widgets
{
public:
    bool key_input(std::string name, char* buf, int buf_size, bool is_password, bool* show_pwd = nullptr, float width = 0.f);
    inline bool key_input(std::string name, char* buf, int buf_size) {
        return key_input(name, buf, buf_size, false, nullptr, 0.f);
    }
    bool sign_in_button(std::string name, bool is_loading, float progress = 0.f, float width = 345.f);
    void sign_in_button(std::string name, bool* show_popup, bool* block);
    bool popup(std::string id_text, bool* show, std::string icon, std::string name, std::string desc, std::string desc2, std::string button_text);
    bool begin_tab_button(std::string name, std::string icon, int tab, int buttons);
    void end_tab_button();
    bool product_button(std::string id, std::string name, std::string desc, std::string flag, std::string flag_icon, ImTextureID person, float width, bool locked = false);
    void lang_selector(float* height);
    bool contact_button(std::string name, std::string icon);
    bool inject(std::string id_text, bool* show, std::string name, float alpha);
};

inline std::unique_ptr<c_widgets> widgets = std::make_unique<c_widgets>();

enum notify_type
{
    success = 0,
    warning = 1,
    error = 2
};

struct notify_state
{
    int notify_id;
    std::string_view text;
    notify_type type{ success };

    ImVec2 window_size{ 0, 0 };
    float notify_alpha{ 0 };
    bool active_notify{ true };
    float notify_timer{ 0 };
    float notify_pos{ 0 };
};

class c_notify
{
public:
    void setup_notify();

    void add_notify(std::string_view text, notify_type type);

private:
    ImVec2 render_notify(int cur_notify_value, float notify_alpha, float notify_percentage, float notify_pos, std::string_view text, notify_type type);

    float notify_time{ 15 };
    int notify_count{ 0 };

    float notify_spacing{ 20 };
    ImVec2 notify_padding{ 20, 20 };

    std::vector<notify_state> notifications;

};

inline std::unique_ptr<c_notify> notify = std::make_unique<c_notify>();
