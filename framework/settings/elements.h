#pragma once
#include <string>
#include "imgui.h"

class c_elements
{
public:

    struct 
    {
        std::string name{ "velocy.ru" };
        c_vec2 size = { 785, 500 };
        c_vec2 padding = { 0, 0 };
        float rounding{ 18 };
    } window;

    struct 
    {
        c_vec2 size = { 785, 60 };
    } titlebar;

    struct 
    {
        c_vec2 size = { 785, 30 };
    } content1;

    struct 
    {
        c_vec2 size = { 785, 410 };
        c_vec2 padding = { 15, 15 };
    } content2;

    struct 
    {
        c_vec2 size = { 30, 380 };
    } tab_bar;

    struct 
    {
        c_vec2 size = { 430, 380 };
    } login_form;

    struct 
    {
        c_vec2 size = { 265, 380 };
    } login_image;
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
