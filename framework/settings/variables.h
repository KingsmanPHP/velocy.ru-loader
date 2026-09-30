#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "../headers/flags.h"
#include <D3DX11.h>
#include <Windows.h>

class c_variables
{
public:

	struct
	{
		float dpi = 1.f;
		int stored_dpi = 100;
		bool dpi_changed = true;
	} gui;

	struct 
	{
		int active = 0;
		int stored = 0;
		float alpha = 1.f;
	} tab;

	struct 
	{
		// ru == 0, us == 1;
		int lang = 0;
	} lang;

	struct
	{
		HWND hwnd;
		RECT rc;
	} winapi;

	struct 
	{
		ID3D11ShaderResourceView* content = nullptr;
		ID3D11ShaderResourceView* ru_flag = nullptr;
		ID3D11ShaderResourceView* us_flag = nullptr;
		ID3D11ShaderResourceView* owl_logo = nullptr;
		ID3D11ShaderResourceView* umbrella_logo = nullptr;
		ID3D11ShaderResourceView* logo = nullptr;

		ID3D11ShaderResourceView* voids = nullptr;
		ID3D11ShaderResourceView* sven = nullptr;
		ID3D11ShaderResourceView* pl = nullptr;
		ID3D11ShaderResourceView* jg = nullptr;
		ID3D11ShaderResourceView* product_layout = nullptr;
		ID3D11ShaderResourceView* fivem = nullptr;
		ID3D11ShaderResourceView* fortnite = nullptr;
	} img;

	gui_style style;

};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
