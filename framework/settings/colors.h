#pragma once
#include "../headers/includes.h"
#include "../headers/flags.h"
#include <memory>

class c_colors
{
public:
    c_col layout = { 17, 17, 19 };
    c_col accent = { 224, 67, 101 };
    c_col white = { 255, 255, 255 };
    c_col black = { 0, 0, 0 };
    c_col text_inactive = { 64, 64, 78 };
    c_col child = { 32, 32, 38 };
    c_col button = { 49, 49, 55 };
    c_col lang_color = { 33, 33, 44 };
    c_col loading_color = { 30, 30, 35 };
};

inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();
