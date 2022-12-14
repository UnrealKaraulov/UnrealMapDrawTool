// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#define GLEW_STATIC
#include <stdio.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include "font.h"
#include "glew.h"

#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imstb_truetype.h"
#include "ImFileDialog.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif


//#define RUSSIAN_LANGUAGE


static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "Glfw Error:" << error << "," << description << std::endl;
}

enum cell_type :unsigned char
{
	cell_none = 0,
	cell_brush,
	cell_hostage,
	cell_player_TT,
	cell_player_CT,
	cell_light,
	cell_buyzone,
	cell_bombzone,
	cell_waterzone
};

struct cell
{
	unsigned char height;
	unsigned char height_offset;
	cell_type type;
};

std::vector<cell> cell_list;


ImVec4 get_cell_color(cell_type c_type)
{
	switch (c_type)
	{
	case cell_none:
		return ImVec4{ 0.2f, 0.3f, 0.4f, 1.0f };
	case cell_brush:
		return ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };
	case cell_hostage:
		return ImVec4{ 0.5f, 0.75f, 1.0f, 1.0f };
	case cell_player_TT:
		return ImVec4{ 0.9f, 0.0f, 0.0f, 1.0f };
	case cell_player_CT:
		return ImVec4{ 0.0f, 0.0f, 0.9f, 1.0f };
	case cell_light:
		return ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
	case cell_buyzone:
		return ImVec4{ 0.0f, 0.75f, 0.75f, 1.0f };
	case cell_bombzone:
		return ImVec4{ 1.0f, 0.5f, 0.0f, 1.0f };
	case cell_waterzone:
		return ImVec4{ 0.2f, 0.6f, 1.0f, 1.0f };
	default: //default
		break;
	}
	return ImVec4(0.0, 0.0, 0.0, 1.0);
}

ImVec4 get_cell_color(cell c)
{
	cell_type c_type = c.type;
	return get_cell_color(c_type);
}

ImVec4 get_cell_color(int id)
{
	cell_type c_type = cell_list[id].type;
	return get_cell_color(c_type);
}

std::string GenerateCuboid(float x1, float y1, float z1, float x2, float y2, float z2, const char* texture = "AAATRIGGER")
{
	std::stringstream outcuboid;

	outcuboid << "{" << std::endl;
	outcuboid << "( " << x2 << " " << y1 << " " << z2 << " ) ( " << x2 << " " << y1 << " " << z1 << " ) ( " << x2 << " " << y2 << " " << z2 << " ) " << texture << " [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1" << std::endl;
	outcuboid << "( " << x1 << " " << y2 << " " << z2 << " ) ( " << x1 << " " << y2 << " " << z1 << " ) ( " << x1 << " " << y1 << " " << z2 << " ) " << texture << " [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1" << std::endl;
	outcuboid << "( " << x2 << " " << y2 << " " << z2 << " ) ( " << x2 << " " << y2 << " " << z1 << " ) ( " << x1 << " " << y2 << " " << z2 << " ) " << texture << " [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1" << std::endl;
	outcuboid << "( " << x1 << " " << y1 << " " << z2 << " ) ( " << x1 << " " << y1 << " " << z1 << " ) ( " << x2 << " " << y1 << " " << z2 << " ) " << texture << " [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1" << std::endl;
	outcuboid << "( " << x1 << " " << y1 << " " << z1 << " ) ( " << x1 << " " << y2 << " " << z1 << " ) ( " << x2 << " " << y1 << " " << z1 << " ) " << texture << " [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1" << std::endl;
	outcuboid << "( " << x2 << " " << y2 << " " << z2 << " ) ( " << x1 << " " << y2 << " " << z2 << " ) ( " << x2 << " " << y1 << " " << z2 << " ) " << texture << " [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1" << std::endl;
	outcuboid << "}";

	return outcuboid.str();
}

std::string GenerateOriginString(float x, float y, float z)
{
	std::stringstream outcuboid;
	outcuboid << "\"origin\" \"" << x << " " << y << " " << z << "\"";
	return outcuboid.str();
}

float GetMinZ_fromPercent(float z, float cell_height, float z_offset)
{
	return z + cell_height / 100.0f * z_offset;
}

float GetMaxZ_fromPercent(float z, float cell_height, float z_offset, float z_height)
{
	return z + (cell_height / 100.0f * z_offset) + (cell_height / 100.0f * z_height);
}

float GetHeight_fromPercent(float cell_height, float z_height)
{
	return cell_height / 100.0f * z_height;
}

float GetHeightOffset_fromPercent(float cell_height, float z_offset)
{
	return cell_height / 100.0f * z_offset;
}

bool UseSkyBorders = false;


void GenerateUnrealMap(std::string fpath, float cell_size, float cell_height, float cell_x, float cell_y, int cell_levels, int cell_layers)
{
	int cur_item = 0;

	int x_min = 99999;
	int x_max = -99999;

	int y_min = 99999;
	int y_max = -99999;


	// search mins/maxs
	for (int lvl = 0; lvl < cell_levels; lvl++)
	{
		for (int layer = 0; layer < cell_layers; layer++)
		{
			for (int y = 0; y < cell_y; y++)
			{
				for (int x = 0; x < cell_x; x++)
				{
					cell cur_cell = cell_list[cur_item];

					if (cur_cell.type != cell_type::cell_none)
					{
						if (x >= x_max)
							x_max = x;
						if (x <= x_min)
							x_min = x;

						if (y >= y_max)
							y_max = y;
						if (y <= y_min)
							y_min = y;
					}
					cur_item++;
				}
			}
		}
	}

	std::stringstream output_bruhes;
	std::stringstream output_entities;

	int tmp_item = 0;
	cur_item = 0;

	int cur_item_y_multiple = 1;
	int cur_item_x_multiple = 1;

	bool one_Light_found = false;

	int lvl_save, layer_save, x_save, y_save;

	float item_z_offset = -(cell_height * cell_levels / 2.0f);
	float item_x_offset = -(cell_size * (abs(x_min) + abs(x_max) + 1) / 2.0f);
	float item_y_offset = cell_size * (abs(y_min) + abs(y_max) + 1) / 2.0f;


	output_bruhes <<
		GenerateCuboid(item_x_offset, item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset,
			-item_y_offset,
			item_z_offset, "CRETE4_FLR02");
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset, item_y_offset,
			-item_z_offset,
			-item_x_offset,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : "CRETE4_WALL01C");
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, item_y_offset,
			item_z_offset - cell_size,
			item_x_offset,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : "CRETE4_WALL01C");
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(-item_x_offset, item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : "CRETE4_WALL01C");
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, item_y_offset + cell_size,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : "CRETE4_WALL01C");
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, -item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			-item_y_offset - cell_size,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : "CRETE4_WALL01C");
	output_bruhes << std::endl;


	for (int lvl = 0; lvl < cell_levels; lvl++)
	{
		for (int layer = 0; layer < cell_layers; layer++)
		{
			for (int y = 0; y < cell_y; y++)
			{
				for (int x = 0; x < cell_x; x++)
				{
					cell cur_cell = cell_list[cur_item];

					if (cur_cell.type != cell_type::cell_none)
					{
						tmp_item = cur_item;
						lvl_save = lvl;
						layer_save = layer;
						x_save = x;
						y_save = y;

						cur_item_y_multiple = 1;
						cur_item_x_multiple = 1;

						// search X multiple
						if (x + 1 < cell_x)
						{
							bool ignore_other = false;
							x++;
							tmp_item++;
							for (; x < cell_x; x++)
							{
								cell next_cell = cell_list[tmp_item];
								if (!ignore_other && next_cell.type == cur_cell.type &&
									next_cell.height == cur_cell.height &&
									next_cell.height_offset == cur_cell.height_offset && (cur_cell.type == cell_type::cell_bombzone
										|| cur_cell.type == cell_type::cell_buyzone || cur_cell.type == cell_type::cell_brush
										|| cur_cell.type == cell_type::cell_waterzone))
								{
									cur_item_x_multiple++;
									cell_list[tmp_item].height = 0;
									cell_list[tmp_item].height_offset = 0;
									cell_list[tmp_item].type = cell_type::cell_none;
								}
								else
								{
									ignore_other = true;
								}
								tmp_item++;
							}
						}

						bool y_search = true;
						while (y + 1 < cell_y && y_search)
						{
							std::vector<int> items_for_erase;
							y++;
							for (x = 0; x < x_save; x++)
							{
								tmp_item++;
							}
							int tmp_x_multiple = 0;

							bool ignore_other = false;
							for (; x < cell_x; x++)
							{
								cell next_cell = cell_list[tmp_item];
								if (tmp_x_multiple < cur_item_x_multiple && !ignore_other && next_cell.type == cur_cell.type &&
									next_cell.height == cur_cell.height &&
									next_cell.height_offset == cur_cell.height_offset && (cur_cell.type == cell_type::cell_bombzone
										|| cur_cell.type == cell_type::cell_buyzone || cur_cell.type == cell_type::cell_brush
										|| cur_cell.type == cell_type::cell_waterzone))
								{
									tmp_x_multiple++;
									items_for_erase.push_back(tmp_item);
								}
								else
								{
									ignore_other = true;
								}
								tmp_item++;
							}


							if (tmp_x_multiple == cur_item_x_multiple)
							{
								cur_item_y_multiple++;
								for (int erase_cell_id : items_for_erase)
								{
									cell_list[erase_cell_id].height = 0;
									cell_list[erase_cell_id].height_offset = 0;
									cell_list[erase_cell_id].type = cell_type::cell_none;
								}
							}
							else
								break;
						}

						lvl = lvl_save;
						layer = layer_save;
						x = x_save;
						y = y_save;

						if (cur_cell.type == cell_type::cell_brush)
						{
							output_bruhes <<
								GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
									GetMinZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset),
									item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
									item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
									GetMaxZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset, (float)cur_cell.height), "TNNL_FLR7");
							output_bruhes << std::endl;
						}
						else
						{
							output_entities << "{" << std::endl;
							if (cur_cell.type == cell_type::cell_player_CT
								|| cur_cell.type == cell_type::cell_player_TT
								|| cur_cell.type == cell_type::cell_hostage
								|| cur_cell.type == cell_type::cell_light)
							{
								if (cur_cell.type == cell_type::cell_player_CT)
									output_entities << "\"classname\" \"info_player_start\"" << std::endl;
								else if (cur_cell.type == cell_type::cell_player_TT)
									output_entities << "\"classname\" \"info_player_deathmatch\"" << std::endl;
								else if (cur_cell.type == cell_type::cell_light)
								{
									one_Light_found = true;
									output_entities << "\"classname\" \"light\"" << std::endl;
									output_entities << "\"_falloff\" \"0\"" << std::endl;
									output_entities << "\"_fade\" \"1.0\"" << std::endl;
									output_entities << "\"style\" \"0\"" << std::endl;
									output_entities << "\"_light\" \"255 255 128 200\"" << std::endl;
								}
								else
									output_entities << "\"classname\" \"hostage_entity\"" << std::endl;
								output_entities << GenerateOriginString(item_x_offset + cell_size / 2.0f + cell_size * x, item_y_offset - cell_size / 2.0f - cell_size * y,
									cur_cell.type == cell_type::cell_light && cur_cell.height_offset > 0 ? GetMinZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset) : item_z_offset + cell_height / 2.0f);
								output_entities << std::endl;
							}
							else if (cur_cell.type == cell_type::cell_buyzone || cur_cell.type == cell_type::cell_bombzone
								|| cur_cell.type == cell_type::cell_waterzone)
							{
								if (cur_cell.type == cell_type::cell_buyzone)
									output_entities << "\"classname\" \"func_buyzone\"" << std::endl;
								else if (cur_cell.type == cell_type::cell_waterzone)
								{
									output_entities << "\"classname\" \"func_water\"" << std::endl;
									output_entities << "\"skin\" \"-3\"" << std::endl;
								}
								else
									output_entities << "\"classname\" \"func_bomb_target\"" << std::endl;
								output_entities <<
									GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
										GetMinZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset),
										item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
										item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
										GetMaxZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset, (float)cur_cell.height), cur_cell.type == cell_type::cell_waterzone ? "!WATER2B" : "AAATRIGGER");
								output_entities << std::endl;
							}
							output_entities << "}" << std::endl;
						}
					}
					cur_item++;
				}
			}
		}

		item_z_offset += cell_height;
	}

	std::stringstream outputmap;
	outputmap << "{" << std::endl;
	outputmap << "\"classname\" \"worldspawn\"" << std::endl;
	outputmap << "\"mapversion\" \"220\"" << std::endl;
	outputmap << "\"startdark\" \"0\"" << std::endl;
	if (!one_Light_found)
	{
		outputmap << "\"light\" \"1\"" << std::endl;
		outputmap << "\"_minlight\" \"0.5\"" << std::endl;
	}
	outputmap << "\"MaxRange\" \"8096\"" << std::endl;
	outputmap << "\"sounds\" \"1\"" << std::endl;
	outputmap << "\"wad\" \"/valve/halflife.wad;/valve/liquids.wad\"" << std::endl;
	outputmap << "\"_generator\" \"UnrealMapDrawTool\"" << std::endl;
	outputmap << output_bruhes.str();
	outputmap << "}" << std::endl;
	outputmap << output_entities.str();

	std::ofstream outFile;
	outFile.open(fpath, std::ios::out);
	if (outFile.is_open())
	{
		outFile << outputmap.rdbuf();
		outFile.close();
	}
}


// ???????????? ?????????? ????????????
char cell_size[256] = "32";
// ???????????? ?????????? ????????????
char cell_height[256] = "128";
// ???????????????????? ?????????? ???? X
char cell_x[256] = "64";
// ???????????????????? ?????????? ???? Y
char cell_y[256] = "64";
// ???????????????????? ??????????????
char cell_levels[256] = "2";
// ???????????????????? ?????????? ???? ???????? ??????????????
char cell_layers[256] = "3";

std::string tmpMapPath = "";

bool setup_end = false;

// ???????????? ?????????????????? ?????? ???????????? ???????????? ?? ??????????????????

#ifdef RUSSIAN_LANGUAGE
const char* items[] = { "??????????", "????????", "????????????????", "????????????", "??????????-????????????", "????????", "???????? ??????????????", "?????????? ????????", "????????" };
cell_type items_types[] = { cell_type::cell_none, cell_type::cell_brush, cell_type::cell_hostage, cell_type::cell_player_TT, cell_type::cell_player_CT,
		cell_type::cell_light, cell_type::cell_buyzone, cell_type::cell_bombzone, cell_type::cell_waterzone };
const char* current_item = "??????????";
#else 
const char* current_item = "NONE";
const char* items[] = { "NONE", "BRUSH", "HOSTAGE", "TERRORIST", "COUNTER-TERRORIST", "LIGHT", "BUYZONE BRUSH", "BOMBZONE BRUSH", "WATER BRUSH" };
cell_type items_types[] = { cell_type::cell_none, cell_type::cell_brush, cell_type::cell_hostage, cell_type::cell_player_TT, cell_type::cell_player_CT,
		cell_type::cell_light, cell_type::cell_buyzone, cell_type::cell_bombzone, cell_type::cell_waterzone };
#endif

cell_type c_type = cell_type::cell_none;
char cur_cell_height[256] = "100";
char cur_cell_height_offset[256] = "0";
bool fill_current_layer = false;
bool clear_current_layer = false;

float scroll_x = 0.0f;
float scroll_y = 0.0f;
int last_layer_opened = 0;

int atoi_val;
const char* atoint_static(const char* s)
{
	atoi_val = atoi(s);
	return (const char*)&atoi_val;
}

void DrawUnrealGUI()
{

	if (!setup_end)
	{
		ImGui::SetNextWindowPos(ImVec2(350.0f, 150.0f), ImGuiCond_FirstUseEver);
#ifdef RUSSIAN_LANGUAGE

		ImGui::Begin("?????????????????? ??????????????????", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("???????????????????? ?????????? ???? X");
		ImGui::InputText("##text1", cell_x, sizeof(cell_x));
		ImGui::Text("???????????????????? ?????????? ???? Y");
		ImGui::InputText("##text2", cell_y, sizeof(cell_y));
		ImGui::Text("???????????? ???????????? ?? ????????????");
		ImGui::InputText("##text3", cell_size, sizeof(cell_size));
		ImGui::Text("???????????? ???????????? ?? ????????????");
		ImGui::InputText("##text4", cell_height, sizeof(cell_height));
		ImGui::Text("???????????????????? ?????????????? ??????????");
		ImGui::InputText("##text5", cell_levels, sizeof(cell_levels));
		ImGui::Text("?????????? ???? ???????? ??????????????");
		ImGui::InputText("##text6", cell_layers, sizeof(cell_layers));
#else 
		ImGui::Begin("Setup", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Cell X count");
		ImGui::InputText("##text1", cell_x, sizeof(cell_x));
		ImGui::Text("Cell Y count");
		ImGui::InputText("##text2", cell_y, sizeof(cell_y));
		ImGui::Text("Cell SIZE in units");
		ImGui::InputText("##text3", cell_size, sizeof(cell_size));
		ImGui::Text("Cell HEIGHT in units");
		ImGui::InputText("##text4", cell_height, sizeof(cell_height));
		ImGui::Text("Levels");
		ImGui::InputText("##text5", cell_levels, sizeof(cell_levels));
		ImGui::Text("Layers for one level");
		ImGui::InputText("##text6", cell_layers, sizeof(cell_layers));
#endif
		if (atoi(cell_x) < 4)
		{
			snprintf(cell_x, sizeof(cell_x), "%d", 4);
		}
		else if (atoi(cell_x) > 512)
		{
			snprintf(cell_x, sizeof(cell_x), "%d", 512);
		}

		if (atoi(cell_y) < 4)
		{
			snprintf(cell_y, sizeof(cell_y), "%d", 4);
		}
		else if (atoi(cell_y) > 512)
		{
			snprintf(cell_y, sizeof(cell_y), "%d", 512);
		}

		if (atoi(cell_size) < 4)
		{
			snprintf(cell_size, sizeof(cell_size), "%d", 4);
		}
		else if (atoi(cell_size) > 256)
		{
			snprintf(cell_size, sizeof(cell_size), "%d", 256);
		}

		if (atoi(cell_height) < 16)
		{
			snprintf(cell_height, sizeof(cell_height), "%d", 16);
		}
		else if (atoi(cell_height) > 512)
		{
			snprintf(cell_height, sizeof(cell_height), "%d", 512);
		}

		if (atoi(cell_levels) < 1)
		{
			snprintf(cell_levels, sizeof(cell_levels), "%d", 1);
		}
		else if (atoi(cell_levels) > 64)
		{
			snprintf(cell_levels, sizeof(cell_levels), "%d", 64);
		}

		if (atoi(cell_layers) < 1)
		{
			snprintf(cell_layers, sizeof(cell_layers), "%d", 1);
		}
		else if (atoi(cell_layers) > 32)
		{
			snprintf(cell_layers, sizeof(cell_layers), "%d", 32);
		}
#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("?????????? ??????????"))
#else
		if (ImGui::Button("START NEW"))
#endif
		{
			cell tmpcell = cell();
			tmpcell.height = 0;
			tmpcell.height_offset = 0;
			tmpcell.type = cell_none;
			cell_list.clear();

			for (int lvl = 0; lvl < atoi(cell_levels); lvl++)
			{
				for (int layer = 0; layer < atoi(cell_layers); layer++)
				{
					for (int y = 0; y < atoi(cell_y); y++)
					{
						for (int x = 0; x < atoi(cell_x); x++)
						{
							cell_list.push_back(tmpcell);
						}
					}
				}
			}
			setup_end = true;
		}
		ImGui::SameLine();

		if (ifd::FileDialog::Instance().IsDone("MapOpenDialog")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::filesystem::path res = ifd::FileDialog::Instance().GetResult();
				int tmp_int_value = 0;
				std::ifstream tmpmap(res.string(), std::ios::in | std::ios::binary);
				if (tmpmap.is_open())
				{
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_x, sizeof(cell_x), "%d", tmp_int_value);
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_y, sizeof(cell_y), "%d", tmp_int_value);
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_size, sizeof(cell_size), "%d", tmp_int_value);
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_height, sizeof(cell_height), "%d", tmp_int_value);
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_levels, sizeof(cell_levels), "%d", tmp_int_value);
					tmpmap.read((char*)&tmp_int_value, 4);
					snprintf(cell_layers, sizeof(cell_layers), "%d", tmp_int_value);

					cell tmpcell = cell();
					tmpcell.height = 0;
					tmpcell.height_offset = 0;
					tmpcell.type = cell_none;
					cell_list.clear();

					for (int lvl = 0; lvl < atoi(cell_levels); lvl++)
					{
						for (int layer = 0; layer < atoi(cell_layers); layer++)
						{
							for (int y = 0; y < atoi(cell_y); y++)
							{
								for (int x = 0; x < atoi(cell_x); x++)
								{
									tmpmap.read((char*)&tmpcell.height, 1);
									tmpmap.read((char*)&tmpcell.height_offset, 1);
									tmpmap.read((char*)&tmpcell.type, 1);
									cell_list.push_back(tmpcell);
								}
							}
						}
					}


					int skybool = UseSkyBorders ? 1 : 0;
					tmpmap.read((char*)&skybool, 4);

					UseSkyBorders = skybool != 0;

					setup_end = true;
					tmpmap.close();
				}
			}
			ifd::FileDialog::Instance().Close();
		}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("?????????????????? ??????????"))
#else
		if (ImGui::Button("LOAD MAP"))
#endif
		{
			ifd::FileDialog::Instance().Open("MapOpenDialog", "Open a map", "Map file (*.umd){.umd},.*", false, tmpMapPath);
		}

		ImGui::End();
	}

	if (setup_end)
	{
		ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
#ifdef RUSSIAN_LANGUAGE
		ImGui::Begin("???????????? ????????????????????????", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("?????????????? ?????????? ???????????? ???????? ?????? ??????????????????");
		ImGui::Text("?????????????? ???????????? ???????????? ???????? ?????? ??????????????");
		ImGui::Separator();
		ImGui::Text("???????????????? ?????? ????????????:");
#else 
		ImGui::Begin("DRAW BAR", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Press left mouse to fill cell");
		ImGui::Text("Press right mouse to clear cell");
		ImGui::Separator();
		ImGui::Text("Select cell type:");
#endif
		if (ImGui::BeginCombo("##text7", current_item))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				bool is_selected = (current_item == items[n]);
				if (ImGui::Selectable(items[n], is_selected))
				{
					current_item = items[n];
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
					for (int m = 0; m < IM_ARRAYSIZE(items_types); m++)
					{
						if (n == m)
						{
							c_type = items_types[m];
						}
					}
				}
			}
			ImGui::EndCombo();
		}
#ifdef RUSSIAN_LANGUAGE
		ImGui::Text("???????????????????? ???? ?????????? (0-100%):");
		ImGui::InputText("##text9", cur_cell_height_offset, sizeof(cur_cell_height_offset));
		ImGui::Text("???????????? ?? ?????????????????? (0-100%):");
		ImGui::InputText("##text8", cur_cell_height, sizeof(cur_cell_height));
#else 
		ImGui::Text("Select cell start Z in percent(0-100):");
		ImGui::InputText("##text9", cur_cell_height_offset, sizeof(cur_cell_height_offset));
		ImGui::Text("Select cell height in percent(0-100):");
		ImGui::InputText("##text8", cur_cell_height, sizeof(cur_cell_height));
#endif
		if (atoi(cur_cell_height_offset) < 0)
		{
			snprintf(cur_cell_height_offset, sizeof(cur_cell_height_offset), "%d", 0);
		}
		else if (atoi(cur_cell_height_offset) > 99)
		{
			snprintf(cur_cell_height_offset, sizeof(cur_cell_height_offset), "%d", 99);
		}

		if (atoi(cur_cell_height) > 100 - atoi(cur_cell_height_offset))
		{
			snprintf(cur_cell_height, sizeof(cur_cell_height), "%d", (100 - atoi(cur_cell_height_offset)));
		}

		if (atoi(cur_cell_height) < 1)
		{
			snprintf(cur_cell_height, sizeof(cur_cell_height), "%d", 1);
		}

		if (c_type == cell_type::cell_waterzone && atoi(cur_cell_height) == 100 && atoi(cur_cell_height_offset) == 0)
		{
			snprintf(cur_cell_height, sizeof(cur_cell_height), "%d", 99);
		}

		ImGui::Checkbox("Skybox", &UseSkyBorders);
#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("???????????? ???????? ????????"))
#else 
		if (ImGui::Button("Fill current layer"))
#endif
		{
			fill_current_layer = true;
		}

		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("???????????????? ??????"))
#else
		if (ImGui::Button("Clear map"))
#endif
		{
			cell tmpcell = cell();
			tmpcell.height = 0;
			tmpcell.height_offset = 0;
			tmpcell.type = cell_none;
			cell_list.clear();

			for (int lvl = 0; lvl < atoi(cell_levels); lvl++)
			{
				for (int layer = 0; layer < atoi(cell_layers); layer++)
				{
					for (int y = 0; y < atoi(cell_y); y++)
					{
						for (int x = 0; x < atoi(cell_x); x++)
						{
							cell_list.push_back(tmpcell);
						}
					}
				}
			}
		}

		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("???????????????? ????????"))
#else
		if (ImGui::Button("Clear layer"))
#endif
		{
			clear_current_layer = true;
		}


		ImGui::Separator();

		if (ifd::FileDialog::Instance().IsDone("MapGenDialog")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::filesystem::path res = ifd::FileDialog::Instance().GetResult();
				std::vector<cell> tmp_cell_list = cell_list;
				GenerateUnrealMap(res.string(), (float)atof(cell_size), (float)atof(cell_height), (float)atof(cell_x), (float)atof(cell_y), atoi(cell_levels), atoi(cell_layers));
				cell_list = tmp_cell_list;
			}
			ifd::FileDialog::Instance().Close();
		}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("?????????????????? .map"))
#else 
		if (ImGui::Button("Generate map!"))
#endif
		{
			ifd::FileDialog::Instance().Save("MapGenDialog", "Generate .map file", "Map file (*.map){.map},.*", tmpMapPath);
		}
		ImGui::SameLine();


		int cur_item = 0;
		if (ifd::FileDialog::Instance().IsDone("MapSaveDialog")) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::filesystem::path res = ifd::FileDialog::Instance().GetResult();
				std::ofstream tmpmap(res.string(), std::ios::out | std::ios::binary);
				if (tmpmap.is_open())
				{
					tmpmap.write((const char*)atoint_static(cell_x), 4);
					tmpmap.write((const char*)atoint_static(cell_y), 4);
					tmpmap.write((const char*)atoint_static(cell_size), 4);
					tmpmap.write((const char*)atoint_static(cell_height), 4);
					tmpmap.write((const char*)atoint_static(cell_levels), 4);
					tmpmap.write((const char*)atoint_static(cell_layers), 4);

					for (int lvl = 0; lvl < atoi(cell_levels); lvl++)
					{
						for (int layer = 0; layer < atoi(cell_layers); layer++)
						{
							for (int y = 0; y < atoi(cell_y); y++)
							{
								for (int x = 0; x < atoi(cell_x); x++)
								{
									cell tmpcell = cell_list[cur_item];
									unsigned char tmpc_type = (unsigned char)tmpcell.type;
									tmpmap.write((const char*)&tmpcell.height, 1);
									tmpmap.write((const char*)&tmpcell.height_offset, 1);
									tmpmap.write((const char*)&tmpc_type, 1);
									cur_item++;
								}
							}
						}
					}

					int skybool = UseSkyBorders ? 1 : 0;
					tmpmap.write((const char*)&skybool, 4);

					tmpmap.close();
				}
			}
			ifd::FileDialog::Instance().Close();
		}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("?????????????????? .umd"))
#else
		if (ImGui::Button("Save map!"))
#endif
		{
			ifd::FileDialog::Instance().Save("MapSaveDialog", "Save a map", "Map file (*.umd){.umd},.*", tmpMapPath);
		}


		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("?????????????? ????????????"))
#else 
		if (ImGui::Button("Close map"))
#endif
		{
			setup_end = false;;
		}

		ImGui::End();
		ImGui::SetNextWindowPos(ImVec2(350.0, 5.0), ImGuiCond_::ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(600.0, 400.0), ImGuiCond_::ImGuiCond_FirstUseEver);
		ImGui::Begin("DRAW MAP", nullptr/*, ImGuiWindowFlags_AlwaysVerticalScrollbar
			| ImGuiWindowFlags_AlwaysHorizontalScrollbar*/);

		ImGui::BeginTabBar("##text10", ImGuiTabBarFlags_FittingPolicyScroll);

		cur_item = 0;

		static ImGuiWindow* cur_bar = nullptr;

		for (int lvl = 0; lvl < atoi(cell_levels); lvl++)
		{
			char levelname[64];
#ifdef RUSSIAN_LANGUAGE
			snprintf(levelname, sizeof(levelname), "?????????????? %d##%d", lvl + 1, cur_item);
#else 
			snprintf(levelname, sizeof(levelname), "Level %d##%d", lvl + 1, cur_item);
#endif
			ImGui::SetNextItemWidth(100);

			if (ImGui::BeginTabItem(levelname))
			{
				ImGui::BeginTabBar("##text11", ImGuiTabBarFlags_FittingPolicyScroll);
				for (int layer = 0; layer < atoi(cell_layers); layer++)
				{
#ifdef RUSSIAN_LANGUAGE
					snprintf(levelname, sizeof(levelname), "???????? %d##%d", layer + 1, cur_item);
#else 
					snprintf(levelname, sizeof(levelname), "Layer %d##%d", layer + 1, cur_item);
#endif
					if (ImGui::BeginTabItem(levelname))
					{
						snprintf(levelname, sizeof(levelname), "##level%d", lvl + 1);

						ImGui::BeginChild(levelname, ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar
							| ImGuiWindowFlags_AlwaysHorizontalScrollbar);


						ImGuiWindow* window = ImGui::GetCurrentWindow();
						ImGuiID active_id = ImGui::GetActiveID();
						bool any_scrollbar_active = active_id && (active_id == ImGui::GetWindowScrollbarID(window, ImGuiAxis_X) || active_id == ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y));

						if (cur_bar == nullptr || cur_bar == window)
						{
							scroll_x = ImGui::GetScrollX();
							scroll_y = ImGui::GetScrollY();
						}
						else
						{
							ImGui::SetScrollX(scroll_x);
							ImGui::SetScrollY(scroll_y);
						}
						cur_bar = window;

						for (int y = 0; y < atoi(cell_y); y++)
						{
							for (int x = 0; x < atoi(cell_x); x++)
							{
								char tmplbl[64];
								if (cell_list[cur_item].type == cell_type::cell_hostage ||
									cell_list[cur_item].type == cell_type::cell_player_CT ||
									cell_list[cur_item].type == cell_type::cell_player_TT)
								{
									cell_list[cur_item].height = 100;
									cell_list[cur_item].height_offset = 0;
									snprintf(tmplbl, sizeof(tmplbl), "##item%d", cur_item);
								}
								else if (cell_list[cur_item].type == cell_type::cell_light)
									snprintf(tmplbl, sizeof(tmplbl), "%d##item%d", cell_list[cur_item].height_offset, cur_item);
								else
									snprintf(tmplbl, sizeof(tmplbl), "%d\n%d##item%d", cell_list[cur_item].height, cell_list[cur_item].height_offset, cur_item);

								ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 2.0f));
								ImGui::PushStyleColor(ImGuiCol_Button, get_cell_color(cur_item));

								if (cell_list[cur_item].type == cell_type::cell_light && cell_list[cur_item].height_offset != 0)
									ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0, 0.0, 0.0, 1.0));

								ImGui::Button(tmplbl, ImVec2(30, 30));

								if (cell_list[cur_item].type == cell_type::cell_light && cell_list[cur_item].height_offset != 0)
									ImGui::PopStyleColor();

								ImGui::PopStyleColor();
								ImGui::PopStyleVar();

								if (!any_scrollbar_active && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
								{
									ImGui::BeginTooltip();

#ifdef RUSSIAN_LANGUAGE
									ImGui::Text("??????: %s", items[cell_list[cur_item].type]);
									ImGui::Text("??????????. %d/%d(%d/%d)", x + 1, y + 1, (x + 1) * atoi(cell_size), (y + 1) * atoi(cell_size));
									ImGui::Text("???????????? %d", atoi(cell_size));
#else 
									ImGui::Text("Type: %s", items[cell_list[cur_item].type]);
									ImGui::Text("Pos %d/%d(%d/%d)", x + 1, y + 1, (x + 1)* atoi(cell_size), (y + 1)* atoi(cell_size));
									ImGui::Text("Size %d units", atoi(cell_size));
#endif
									if (cell_list[cur_item].type == cell_type::cell_brush
										|| cell_list[cur_item].type == cell_type::cell_buyzone
										|| cell_list[cur_item].type == cell_type::cell_bombzone
										|| cell_list[cur_item].type == cell_type::cell_waterzone
										|| cell_list[cur_item].type == cell_type::cell_light)
									{
#ifdef RUSSIAN_LANGUAGE
										if (cell_list[cur_item].type != cell_type::cell_light)
											ImGui::Text("???????????? %d", (int)GetHeight_fromPercent((float)atoi(cell_height), cell_list[cur_item].height));
										ImGui::Text("???? ?????????? %d", (int)GetHeightOffset_fromPercent((float)atoi(cell_height), cell_list[cur_item].height_offset));
#else 
										if (cell_list[cur_item].type != cell_type::cell_light)
											ImGui::Text("Height %d units", (int)GetHeight_fromPercent((float)atoi(cell_height), cell_list[cur_item].height));
										ImGui::Text("Height start %d units", (int)GetHeightOffset_fromPercent((float)atoi(cell_height), cell_list[cur_item].height_offset));
#endif
									}
									ImGui::EndTooltip();


									if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
									{
										if (c_type == cell_type::cell_none)
										{
											cell_list[cur_item].type = cell_type::cell_none;
											cell_list[cur_item].height = 0;
											cell_list[cur_item].height_offset = 0;
										}
										else
										{
											cell_list[cur_item].type = c_type;
											cell_list[cur_item].height = (unsigned char)atoi(cur_cell_height);
											cell_list[cur_item].height_offset = (unsigned char)atoi(cur_cell_height_offset);
										}
									}
									else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
									{
										cell_list[cur_item].type = cell_type::cell_none;
										cell_list[cur_item].height = 0;
										cell_list[cur_item].height_offset = 0;
									}
								}

								if (fill_current_layer)
								{
									cell_list[cur_item].type = c_type;
									cell_list[cur_item].height = (unsigned char)atoi(cur_cell_height);
									cell_list[cur_item].height_offset = (unsigned char)atoi(cur_cell_height_offset);
								}
								else if (clear_current_layer)
								{
									cell_list[cur_item].type = cell_type::cell_none;
									cell_list[cur_item].height = 0;
									cell_list[cur_item].height_offset = 0;
								}

								if (x + 1 != atoi(cell_x))
								{
									ImGui::SameLine(0.0, 2.0f);
								}

								cur_item++;
							}
						}
						clear_current_layer = false;
						fill_current_layer = false;
						ImGui::EndChild();
						ImGui::EndTabItem();
					}
					else
					{
						for (int y = 0; y < atoi(cell_y); y++)
						{
							for (int x = 0; x < atoi(cell_x); x++)
							{
								cur_item++;
							}
						}
					}
				}
				ImGui::EndTabBar();
				ImGui::EndTabItem();
			}
			else
			{
				for (int layer = 0; layer < atoi(cell_layers); layer++)
				{
					for (int y = 0; y < atoi(cell_y); y++)
					{
						for (int x = 0; x < atoi(cell_x); x++)
						{
							cur_item++;
						}
					}
				}
			}
		}
		ImGui::EndTabBar();


		ImGui::End();
	}
}



int main(int, char**)
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(960, 620, "Unreal Map Draw Tool 1.10", NULL, NULL);
	if (window == NULL)
		return 1;

	glewInit();

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImFontConfig config;

	config.SizePixels = 13.0f;
	config.OversampleH = 3;
	config.OversampleV = 1;
	config.RasterizerMultiply = 1.5f;
	config.PixelSnapH = true;

	io.Fonts->AddFontFromMemoryCompressedTTF((const char*)compressed_data, compressed_size, 14.0, &config);

	config.MergeMode = true;

	io.Fonts->AddFontFromMemoryCompressedTTF((const char*)compressed_data, compressed_size, 14.0, &config, io.Fonts->GetGlyphRangesCyrillic());

	io.Fonts->AddFontFromMemoryCompressedTTF((const char*)compressed_data, compressed_size, 14.0, &config, io.Fonts->GetGlyphRangesDefault());

	io.Fonts->Build();

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return (void*)tex;
	};

	ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
		GLuint texID = (GLuint)((uintptr_t)tex);
		glDeleteTextures(1, &texID);
	};

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		int display_w, display_h;
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		DrawUnrealGUI();

		ImGui::Render();
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
