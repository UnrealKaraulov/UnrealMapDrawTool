#include <stdio.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <filesystem>

#ifndef JACK_PLUGIN
#include "font.h"
#include "glew.h"

#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imstb_truetype.h"
#include "ImFileDialog.h"
#endif 

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define RUSSIAN_LANGUAGE

static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "Glfw Error:" << error << "," << description << std::endl;
}

enum umd_flags : unsigned int {
	UMD_TEXTURES_SKIP_OPTIMIZE = 1 << 0,
	UMD_OPTIMIZE_DISABLED = 1 << 1
};

enum cell_type :unsigned char
{
	cell_none = 0,
	cell_brush,
	cell_wall,
	cell_hostage,
	cell_player_TT,
	cell_player_CT,
	cell_light,
	cell_buyzone,
	cell_bombzone,
	cell_waterzone
};

int UMD_MAGIC = 'umd2';

struct cell
{
	unsigned char height;
	unsigned char height_offset;
	unsigned char texid;
	cell_type type;
};

std::vector<std::string> texture_list = { "CRETE4_FLR02" };

std::vector<cell> cell_list;

unsigned int cell_flags = 0;

bool cell_edit(cell& c, unsigned char new_height, unsigned char new_offset, cell_type new_type)
{
	if (c.height == new_height && c.height_offset == new_offset && c.type == new_type)
	{
		return false;
	}

	c.height = new_height;
	c.height_offset = new_offset;
	c.type = new_type;

	return true;
}

#ifndef JACK_PLUGIN
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
#endif
std::string GenerateCuboid(float x1, float y1, float z1, float x2, float y2, float z2, std::string texture = "AAATRIGGER")
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

float GetMinZ_fromPercent(float z, int cell_height, float z_offset)
{
	return z + cell_height / 100.0f * z_offset;
}

float GetMaxZ_fromPercent(float z, int cell_height, float z_offset, float z_height)
{
	return z + (cell_height / 100.0f * z_offset) + (cell_height / 100.0f * z_height);
}

float GetHeight_fromPercent(int cell_height, float z_height)
{
	return cell_height / 100.0f * z_height;
}

float GetHeightOffset_fromPercent(int cell_height, float z_offset)
{
	return cell_height / 100.0f * z_offset;
}

bool UseSkyBorders = false;

void GenerateUnrealMap(const std::string& fpath, int cell_size, int cell_height, int cell_x, int cell_y, int cell_levels, int cell_layers, int texid = 0)
{
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
					int index = (lvl * cell_layers + layer) * cell_y * cell_x + y * cell_x + x;
					cell cur_cell = cell_list[index];

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
				}
			}
		}
	}

	std::stringstream output_bruhes;
	std::stringstream output_entities;

	bool one_Light_found = false;

	float item_z_offset = -(cell_height * cell_levels / 2.0f);
	float item_x_offset = -(cell_size * (abs(x_min) + abs(x_max) + 1) / 2.0f);
	float item_y_offset = cell_size * (abs(y_min) + abs(y_max) + 1) / 2.0f;


	output_bruhes <<
		GenerateCuboid(item_x_offset, item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset,
			-item_y_offset,
			item_z_offset, texture_list[texid]);
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset, item_y_offset,
			-item_z_offset,
			-item_x_offset,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : texture_list[texid]);
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, item_y_offset,
			item_z_offset - cell_size,
			item_x_offset,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : texture_list[texid]);
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(-item_x_offset, item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			-item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : texture_list[texid]);
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, item_y_offset + cell_size,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			item_y_offset,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : texture_list[texid]);
	output_bruhes << std::endl;

	output_bruhes <<
		GenerateCuboid(item_x_offset - cell_size, -item_y_offset,
			item_z_offset - cell_size,
			-item_x_offset + cell_size,
			-item_y_offset - cell_size,
			-item_z_offset + cell_size, UseSkyBorders ? "SKY" : texture_list[texid]);
	output_bruhes << std::endl;

	for (int lvl = 0; lvl < cell_levels; lvl++)
	{
		for (int layer = 0; layer < cell_layers; layer++)
		{
			for (int y = 0; y < cell_y; y++)
			{
				for (int x = 0; x < cell_x; x++)
				{
					int index = (lvl * cell_layers + layer) * cell_y * cell_x + y * cell_x + x;
					cell& cur_cell = cell_list[index];

					if (cur_cell.type != cell_type::cell_none)
					{
						int cur_item_y_multiple = 1;
						int cur_item_x_multiple = 1;
						int cur_item_z_multiple = 1;

						// Поиск Z multiple
						if (lvl + 1 < cell_levels && !(cell_flags & umd_flags::UMD_OPTIMIZE_DISABLED))
						{
							for (int multz = lvl + 1; multz < cell_levels; multz++)
							{
								bool match = true;
								for (int multy = 0; multy < cur_item_y_multiple; multy++)
								{
									for (int multx = 0; multx < cur_item_x_multiple; multx++)
									{
										int multindex = (multz * cell_layers + layer) * cell_y * cell_x + (y + multy) * cell_x + x + multx;
										cell& multcell = cell_list[multindex];
										if (!(multcell.type == cur_cell.type &&
											multcell.height == cur_cell.height &&
											multcell.height_offset == cur_cell.height_offset)
											|| (cell_flags & umd_flags::UMD_TEXTURES_SKIP_OPTIMIZE && multcell.texid != cur_cell.texid))
										{
											match = false;
											break;
										}
									}
									if (!match) break;
								}
								if (match)
								{
									cur_item_z_multiple++;
									for (int multy = 0; multy < cur_item_y_multiple; multy++)
									{
										for (int multx = 0; multx < cur_item_x_multiple; multx++)
										{
											int multindex = (multz * cell_layers + layer) * cell_y * cell_x + (y + multy) * cell_x + x + multx;
											cell& multcell = cell_list[multindex];
											multcell.height = 0;
											multcell.height_offset = 0;
											multcell.texid = 0;
											multcell.type = cell_type::cell_none;
										}
									}
								}
								else
								{
									break;
								}
							}
						}

						// Поиск Y multiple (с учетом Z multiple)
						if (y + 1 < cell_y && !(cell_flags & umd_flags::UMD_OPTIMIZE_DISABLED))
						{
							for (int multy = y + 1; multy < cell_y; multy++)
							{
								bool match = true;
								for (int multz = 0; multz < cur_item_z_multiple; multz++)
								{
									int multindex = (lvl + multz) * cell_layers * cell_y * cell_x + multy * cell_x + x;
									cell& multcell = cell_list[multindex];
									if (!(multcell.type == cur_cell.type &&
										multcell.height == cur_cell.height &&
										multcell.height_offset == cur_cell.height_offset) 
										|| (cell_flags & umd_flags::UMD_TEXTURES_SKIP_OPTIMIZE && multcell.texid != cur_cell.texid))
									{
										match = false;
										break;
									}
								}
								if (match)
								{
									cur_item_y_multiple++;
									for (int multz = 0; multz < cur_item_z_multiple; multz++)
									{
										int multindex = (lvl + multz) * cell_layers * cell_y * cell_x + multy * cell_x + x;
										cell& multcell = cell_list[multindex];
										multcell.height = 0;
										multcell.height_offset = 0;
										multcell.texid = 0;
										multcell.type = cell_type::cell_none;
									}
								}
								else
								{
									break;
								}
							}
						}

						// Поиск X multiple (с учетом Y и Z multiple)
						if (x + 1 < cell_x && !(cell_flags & umd_flags::UMD_OPTIMIZE_DISABLED))
						{
							for (int multx = x + 1; multx < cell_x; multx++)
							{
								bool match = true;
								for (int multz = 0; multz < cur_item_z_multiple; multz++)
								{
									for (int multy = 0; multy < cur_item_y_multiple; multy++)
									{
										int multindex = (lvl + multz) * cell_layers * cell_y * cell_x + (y + multy) * cell_x + multx;
										cell& multcell = cell_list[multindex];
										if (!(multcell.type == cur_cell.type &&
											multcell.height == cur_cell.height &&
											multcell.height_offset == cur_cell.height_offset)
											|| (cell_flags & umd_flags::UMD_TEXTURES_SKIP_OPTIMIZE && multcell.texid != cur_cell.texid))
										{
											match = false;
											break;
										}
									}
									if (!match) break;
								}
								if (match)
								{
									cur_item_x_multiple++;
									for (int multz = 0; multz < cur_item_z_multiple; multz++)
									{
										for (int multy = 0; multy < cur_item_y_multiple; multy++)
										{
											int multindex = (lvl + multz) * cell_layers * cell_y * cell_x + (y + multy) * cell_x + multx;
											cell& multcell = cell_list[multindex];
											multcell.height = 0;
											multcell.height_offset = 0;
											multcell.texid = 0;
											multcell.type = cell_type::cell_none;
										}
									}
								}
								else
								{
									break;
								}
							}
						}

						if (cur_cell.type == cell_type::cell_brush)
						{
							if (cur_item_z_multiple > 1)
							{
								output_bruhes <<
									GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
										item_z_offset,
										item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
										item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
										item_z_offset + cell_height * cur_item_z_multiple, cur_cell.texid < texture_list.size() ? texture_list[cur_cell.texid] : "AAATRIGGER");
								output_bruhes << std::endl;
							}
							else
							{
								output_bruhes <<
									GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
										GetMinZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset),
										item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
										item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
										GetMaxZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset, (float)cur_cell.height), cur_cell.texid < texture_list.size() ? texture_list[cur_cell.texid] : "AAATRIGGER");
								output_bruhes << std::endl;
							}
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
								|| cur_cell.type == cell_type::cell_waterzone || cur_cell.type == cell_wall)
							{
								if (cur_cell.type == cell_type::cell_buyzone)
								{
									output_entities << "\"classname\" \"func_buyzone\"" << std::endl;
								}
								else if (cur_cell.type == cell_type::cell_waterzone)
								{
									output_entities << "\"classname\" \"func_water\"" << std::endl;
									output_entities << "\"skin\" \"-3\"" << std::endl;
								}
								else if (cur_cell.type == cell_type::cell_wall)
								{
									output_entities << "\"classname\" \"func_wall\"" << std::endl;
								}
								else
								{
									output_entities << "\"classname\" \"func_bomb_target\"" << std::endl;
								}
								std::string texName = cur_cell.texid < texture_list.size() ? texture_list[cur_cell.texid] : "AAATRIGGER";

								if (cur_cell.type == cell_type::cell_bombzone
									|| cur_cell.type == cell_type::cell_buyzone)
								{
									texName = "AAATRIGGER";
								}

								if (cur_item_z_multiple > 1)
								{
									

									output_entities <<
										GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
											item_z_offset,
											item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
											item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
											item_z_offset + cell_height * cur_item_z_multiple, texName);
									output_entities << std::endl;
								}
								else
								{
									output_entities <<
										GenerateCuboid(item_x_offset + x * cell_size, item_y_offset - y * cell_size,
											GetMinZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset),
											item_x_offset + x * cell_size + cell_size * cur_item_x_multiple,
											item_y_offset - (y * cell_size + cell_size * cur_item_y_multiple),
											GetMaxZ_fromPercent(item_z_offset, cell_height, (float)cur_cell.height_offset, (float)cur_cell.height), texName);
									output_entities << std::endl;
								}
							}
							output_entities << "}" << std::endl;
						}
					}
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


// Размер одной ячейки
char cell_size[256] = "32";
// Высота одной ячейки
char cell_height[256] = "128";
// Количество ячеек по X
char cell_x[256] = "64";
// Количество ячеек по Y
char cell_y[256] = "64";
// Количество уровней
char cell_levels[256] = "2";
// Количество слоев на один уровень
char cell_layers[256] = "3";

std::string tmpMapPath = "";

bool setup_end = false;

// Список доступных для выбора брашей и сущностей

#ifdef RUSSIAN_LANGUAGE
const char* items[] = { "ПУСТО", "БРАШ", "ЗАЛОЖНИК", "ТЕРРОР", "КОНТР-ТЕРРОР", "СВЕТ", "ЗОНА ЗАКУПКИ", "ПЛЕНТ ЗОНА", "ВОДА" };
cell_type items_types[] = { cell_type::cell_none, cell_type::cell_brush, cell_type::cell_hostage, cell_type::cell_player_TT, cell_type::cell_player_CT,
		cell_type::cell_light, cell_type::cell_buyzone, cell_type::cell_bombzone, cell_type::cell_waterzone };
const char* current_item = "ПУСТО";
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



bool LoadMap(const std::string& path)
{
	int tmp_int_value = 0;
	std::ifstream tmpmap(path, std::ios::in | std::ios::binary);
	if (tmpmap && tmpmap.is_open())
	{
		tmpmap.read((char*)&tmp_int_value, 4);
		bool read_textures = true;
		if (tmp_int_value == UMD_MAGIC)
		{
			// 4x zero
			tmpmap.read((char*)&tmp_int_value, 4);
			tmpmap.read((char*)&tmp_int_value, 4);
			tmpmap.read((char*)&tmp_int_value, 4);
			tmpmap.read((char*)&tmp_int_value, 4);


			tmpmap.read((char*)&tmp_int_value, 4);
		}
		else
		{
			read_textures = false;
		}

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
		tmpcell.texid = 0;
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
						tmpmap.read((char*)&tmpcell.texid, 1);
						tmpmap.read((char*)&tmpcell.type, 1);
						cell_list.push_back(tmpcell);
					}
				}
			}
		}


		int skybool = UseSkyBorders ? 1 : 0;
		tmpmap.read((char*)&skybool, 4);

		tmpmap.read((char*)&cell_flags, 4);

		UseSkyBorders = skybool != 0;

		setup_end = true;

		if (read_textures)
		{
			texture_list.clear();
			int textureCount;
			tmpmap.read(reinterpret_cast<char*>(&textureCount), 4);

			for (int i = 0; i < textureCount; ++i) 
			{
				int length;
				tmpmap.read(reinterpret_cast<char*>(&length), 4);
				std::string texture(length, '\0');
				tmpmap.read(&texture[0], length);
				texture_list.push_back(texture);
			}
		}

		tmpmap.close();
		return true;
	}
	return false;
}

#ifndef JACK_PLUGIN


bool AutoSave = false;
bool NeedSaveChanges = false;
std::string LastSaveString = { };


void DrawUnrealGUI()
{

	if (!setup_end)
	{
		ImGui::SetNextWindowPos(ImVec2(350.0f, 150.0f), ImGuiCond_FirstUseEver);
#ifdef RUSSIAN_LANGUAGE

		ImGui::Begin("Начальная настройка", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Количество ячеек по X");
		ImGui::InputText("##text1", cell_x, sizeof(cell_x));
		ImGui::Text("Количество ячеек по Y");
		ImGui::InputText("##text2", cell_y, sizeof(cell_y));
		ImGui::Text("Размер ячейки в юнитах");
		ImGui::InputText("##text3", cell_size, sizeof(cell_size));
		ImGui::Text("Высота ячейки в юнитах");
		ImGui::InputText("##text4", cell_height, sizeof(cell_height));
		ImGui::Text("Количество уровней карты");
		ImGui::InputText("##text5", cell_levels, sizeof(cell_levels));
		ImGui::Text("Слоев на один уровень");
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
		if (ImGui::Button("НОВАЯ КАРТА"))
#else
		if (ImGui::Button("START NEW"))
#endif
		{
			cell tmpcell = cell();
			tmpcell.height = 0;
			tmpcell.height_offset = 0;
			tmpcell.texid = 0;
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
				LoadMap(res.string());
			}
			ifd::FileDialog::Instance().Close();
			}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("ЗАГРУЗИТЬ КАРТУ"))
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
		ImGui::Begin("ПАНЕЛЬ ИНСТРУМЕНТОВ", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Нажмите левую кнопку мыши для рисования");
		ImGui::Text("Нажмите правую кнопку мыши для очистки");
		ImGui::Separator();
		ImGui::Text("Выберите тип ячейки:");
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
		ImGui::Text("Расстояние от земли (0-100%):");
		ImGui::InputText("##text9", cur_cell_height_offset, sizeof(cur_cell_height_offset));
		ImGui::Text("Высота в процентах (0-100%):");
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

		ImGui::SameLine();


		if (LastSaveString.empty())
		{
			ImGui::BeginDisabled();
			AutoSave = false;
		}

		ImGui::Checkbox("Auto save", &AutoSave);


		if (LastSaveString.empty())
		{
			ImGui::EndDisabled();
					}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Залить весь слой"))
#else 
		if (ImGui::Button("Fill current layer"))
#endif
		{
			fill_current_layer = true;
		}

		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Очистить всё"))
#else
		if (ImGui::Button("Clear map"))
#endif
		{
			cell tmpcell = cell();
			tmpcell.height = 0;
			tmpcell.height_offset = 0;
			tmpcell.texid = 0;
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

			NeedSaveChanges = true;
		}

		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Очистить слой"))
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
				GenerateUnrealMap(res.string(), atoi(cell_size), atoi(cell_height), atoi(cell_x), atoi(cell_y), atoi(cell_levels), atoi(cell_layers));
				cell_list = std::move(tmp_cell_list);
			}
			ifd::FileDialog::Instance().Close();
		}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Сохранить .map"))
#else 
		if (ImGui::Button("Generate map!"))
#endif
		{
			ifd::FileDialog::Instance().Save("MapGenDialog", "Generate .map file", "Map file (*.map){.map},.*", tmpMapPath);
		}
		ImGui::SameLine();


		int cur_item = 0;
		if (ifd::FileDialog::Instance().IsDone("MapSaveDialog") || (NeedSaveChanges && !LastSaveString.empty())) {
			if (ifd::FileDialog::Instance().HasResult() || (NeedSaveChanges && !LastSaveString.empty())) {

				if (!NeedSaveChanges)
				{
					std::filesystem::path res = ifd::FileDialog::Instance().GetResult();
					LastSaveString = res.string();
				}
				NeedSaveChanges = false;

				std::ofstream tmpmap(LastSaveString, std::ios::out | std::ios::binary);
				if (tmpmap.is_open())
				{
					tmpmap.write(reinterpret_cast<const char*>(&UMD_MAGIC), 4);
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
									tmpmap.write((const char*)&tmpcell.texid, 1);
									tmpmap.write((const char*)&tmpc_type, 1);
									cur_item++;
								}
							}
						}
					}

					int skybool = UseSkyBorders ? 1 : 0;
					tmpmap.write((const char*)&skybool, 4);

					int textureCount = (int)texture_list.size();
					tmpmap.write(reinterpret_cast<const char*>(&textureCount), 4);

					for (const auto& texture : texture_list) {
						int length = texture.length();
						tmpmap.write(reinterpret_cast<const char*>(&length), 4);
						tmpmap.write(texture.c_str(), length);
					}

					tmpmap.close();
				}
			}
			ifd::FileDialog::Instance().Close();
		}

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Сохранить .umd"))
#else
		if (ImGui::Button("Save map!"))
#endif
		{
			ifd::FileDialog::Instance().Save("MapSaveDialog", "Save a map", "Map file (*.umd){.umd},.*", tmpMapPath);
		}


		ImGui::SameLine();

#ifdef RUSSIAN_LANGUAGE
		if (ImGui::Button("Закрыть проект"))
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
			snprintf(levelname, sizeof(levelname), "Уровень %d##%d", lvl + 1, cur_item);
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
					snprintf(levelname, sizeof(levelname), "Слой %d##%d", layer + 1, cur_item);
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
									if (cell_list[cur_item].height != 100 || cell_list[cur_item].height_offset != 0)
									{
										if (cell_edit(cell_list[cur_item], 100, 0, cell_list[cur_item].type))
										{
											NeedSaveChanges = true;
										}
									}
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
									ImGui::Text("Тип: %s", items[cell_list[cur_item].type]);
									ImGui::Text("Коорд. %d/%d(%d/%d)", x + 1, y + 1, (x + 1) * atoi(cell_size), (y + 1) * atoi(cell_size));
									ImGui::Text("Размер %d", atoi(cell_size));
#else 
									ImGui::Text("Type: %s", items[cell_list[cur_item].type]);
									ImGui::Text("Pos %d/%d(%d/%d)", x + 1, y + 1, (x + 1) * atoi(cell_size), (y + 1) * atoi(cell_size));
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
											ImGui::Text("Высота %d", (int)GetHeight_fromPercent(atoi(cell_height), cell_list[cur_item].height));
										ImGui::Text("От земли %d", (int)GetHeightOffset_fromPercent(atoi(cell_height), cell_list[cur_item].height_offset));
#else 
										if (cell_list[cur_item].type != cell_type::cell_light)
											ImGui::Text("Height %d units", (int)GetHeight_fromPercent(atoi(cell_height), cell_list[cur_item].height));
										ImGui::Text("Height start %d units", (int)GetHeightOffset_fromPercent(atoi(cell_height), cell_list[cur_item].height_offset));
#endif
									}
									ImGui::EndTooltip();


									if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
									{
										if (c_type == cell_type::cell_none)
										{
											if (cell_edit(cell_list[cur_item], 0, 0, cell_type::cell_none))
											{
												NeedSaveChanges = true;
											}
										}
										else
										{
											if (cell_edit(cell_list[cur_item], (unsigned char)atoi(cur_cell_height), (unsigned char)atoi(cur_cell_height_offset), c_type))
											{
												NeedSaveChanges = true;
											}
										}
									}
									else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
									{
										if (cell_edit(cell_list[cur_item], 0, 0, cell_type::cell_none))
										{
											NeedSaveChanges = true;
										}
									}
								}

								if (fill_current_layer)
								{
									cell_list[cur_item].type = c_type;
									cell_list[cur_item].height = (unsigned char)atoi(cur_cell_height);
									cell_list[cur_item].height_offset = (unsigned char)atoi(cur_cell_height_offset);

									if (AutoSave)
									{
										NeedSaveChanges = true;
									}
								}
								else if (clear_current_layer)
								{
									cell_list[cur_item].type = cell_type::cell_none;
									cell_list[cur_item].height = 0;
									cell_list[cur_item].height_offset = 0;
									cell_list[cur_item].texid = 0;

									if (AutoSave)
									{
										NeedSaveChanges = true;
									}
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
#endif


#ifdef JACK_PLUGIN

#include <Windows.h>

const char* actionName = "About";
const char* actionName2 = "Enable AutoReload";
const char* actionDescription = "UnrealMapDrawTool is a map painting tools. Download https://github.com/UnrealKaraulov/UnrealMapDrawTool Extension of maps: .umd. ";
const char* actionDescription2 = "Auto reload .umd map when change!";
const char* actionDirectory = "UnrealMapDrawTool";
const char* actionFormatName = "UnrealMapDrawTool MAP";
const char* actionFormat = ".umd";


DWORD StartTicks = 0;
DWORD MainThread = 0;
HHOOK hhookSysMsg = 0;

bool AutoReload = false;


std::filesystem::file_time_type lastEditTime = {};


std::filesystem::file_time_type GetLastEditTime(std::string path)
{
	return std::filesystem::last_write_time(path);
}

std::string lastFilePath_umd;
std::string lastFilePath_map;

typedef __int64(__fastcall* pvpImport)(int version, const char* src, unsigned char* data);

unsigned char* importData = NULL;

__int64 import_real()
{
	HMODULE mdl = LoadLibraryA("vpHalfLifex64.dll");

	if (!mdl)
		return 0;

	auto proc = GetProcAddress(mdl, "vpImport");
	if (!proc)
		return 0;

	pvpImport imp = (pvpImport)proc;

	// Load .map
	__int64 retval = imp(1, lastFilePath_map.c_str(), importData);

	DeleteFileA(lastFilePath_map.c_str());

	return retval;
}

LRESULT CALLBACK HookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < HC_ACTION)
		return CallNextHookEx(hhookSysMsg, nCode, wParam, lParam);

	if (GetCurrentThreadId() == MainThread && GetTickCount() - StartTicks > 500)
	{
		StartTicks = GetTickCount();
		if (AutoReload && lastFilePath_umd.size())
		{
			if (GetLastEditTime(lastFilePath_umd) != lastEditTime)
			{
				lastEditTime = GetLastEditTime(lastFilePath_umd);
				cell_list.clear();
				if (!LoadMap(lastFilePath_umd))
				{
					lastFilePath_umd = std::string();
				}
				else
				{
					GenerateUnrealMap(lastFilePath_map, atoi(cell_size), atoi(cell_height), atoi(cell_x), atoi(cell_y), atoi(cell_levels), atoi(cell_layers));
					cell_list.clear();

					// Clear cache (but not hastable memory release)
					*(__int64*)(importData + 0x18) = 0;
					*(__int64*)(importData + 0x20) = 0;
					*(__int64*)(importData + 0xC8) = 0;

					if (!import_real())
					{
						lastFilePath_umd = std::string();
					}
				}
			}
		}
	}
	return CallNextHookEx(hhookSysMsg, nCode, wParam, lParam);
}
BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_DETACH)
	{
		UnhookWindowsHookEx(hhookSysMsg);
	}
	else if (fdwReason == DLL_PROCESS_ATTACH)
	{
		MainThread = GetCurrentThreadId();
		hhookSysMsg = SetWindowsHookExW(WH_GETMESSAGE, HookCallWndProc, hinstDLL, GetCurrentThreadId());
	}
	return 1;
}

BOOL CALLBACK PrintAboutForJack(HWND hwnd, LPARAM lParam) {
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);

	DWORD targetProcessId = *reinterpret_cast<DWORD*>(lParam);

	if (processId == targetProcessId) {
		MessageBoxA(hwnd, actionDescription, "UnrealMapDrawTool .umd Import Plugin by Karaulov\nUnreal Map Draw Tool 1.16", 0);
		return FALSE;
	}
	return TRUE;
}

__int64 dispatchFunc()
{
	DWORD processId = GetCurrentProcessId();
	EnumWindows(PrintAboutForJack, reinterpret_cast<LPARAM>(&processId));
	return 0;
}

#pragma pack(push, 1)

struct JACK_ACTION
{
	const char* actionData[3];
	__int64 state; // 
	__int64 flags; // 0 is item enabled 1+ disabled
	void* dispatch_func;
	__int64 unknown;
	__int64 id;
	__int64 reserved;
	__int64 reserved2;
};

#pragma pack(pop)

JACK_ACTION tmpJACK_ACTION{ {actionName,actionDescription,actionDirectory}, 0 , 0, (void*)dispatchFunc, 0,0, 0,0 };

__int64 dispatchFunc2();

JACK_ACTION tmpJACK_ACTION2{ {actionName2,actionName2,actionDirectory}, 0 , 0, (void*)dispatchFunc2, 0, 5, 0,0 };

BOOL CALLBACK PrintUpdateStateForJack(HWND hwnd, LPARAM lParam) {
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);

	DWORD targetProcessId = *reinterpret_cast<DWORD*>(lParam);

	if (processId == targetProcessId) {
		if (AutoReload)
		{
			MessageBoxA(hwnd, "Auto Reload enabled!", "UnrealMapDrawTool .umd Auto Reload enabled!", 0);
		}
		else
		{
			MessageBoxA(hwnd, "Auto Reload disabled!", "UnrealMapDrawTool .umd Auto Reload disabled!", 0);
		}
		return FALSE;
	}
	return TRUE;
}

__int64 dispatchFunc2()
{
	AutoReload = !AutoReload;

	DWORD processId = GetCurrentProcessId();
	EnumWindows(PrintUpdateStateForJack, reinterpret_cast<LPARAM>(&processId));
	return 0;
}

typedef void(__fastcall* jackAddAction)(JACK_ACTION*, unsigned char* qlib);

__int64 __fastcall vpEnumActions(jackAddAction addAction, unsigned char* qlib)
{
	addAction(&tmpJACK_ACTION, qlib);
	addAction(&tmpJACK_ACTION2, qlib);
	return 1;
}

__int64 __fastcall vpMain(void* unused, __int64 sdk_version)
{
	if (sdk_version != 100)
		return 100LL;
	setlocale(0, "C");
	return 0LL;
}

bool __fastcall vpEnumImportFormats(__int64(__fastcall* jackAddImport)(int version, const char* name, const char* extension, unsigned char* qlib), unsigned char* qlib)
{
	return jackAddImport(0, actionFormatName, actionFormat, qlib) != 0;
}

__int64 __fastcall vpImport(int formatid, char* src, unsigned char* jack_data)
{
	char newSrcName[2048];
	sprintf_s(newSrcName, 2048, "%s", src);

	/*char address[2048];
	sprintf_s(address, 2048, "%p", jack_data);
	MessageBoxA(0, address, address, 0);*/

	lastFilePath_umd = newSrcName;

	newSrcName[strlen(src) - 1] = 'p';
	newSrcName[strlen(src) - 2] = 'a';
	newSrcName[strlen(src) - 3] = 'm';
	newSrcName[strlen(src) - 4] = '.';

	lastFilePath_map = newSrcName;

	cell_list.clear();

	if (!LoadMap(lastFilePath_umd))
	{
		lastFilePath_umd = std::string();
		return 0;
	}

	GenerateUnrealMap(lastFilePath_map, atoi(cell_size), atoi(cell_height), atoi(cell_x), atoi(cell_y), atoi(cell_levels), atoi(cell_layers));
	cell_list.clear();

	importData = jack_data;
	return import_real();
				}


#endif

#ifndef JACK_PLUGIN

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
	GLFWwindow* window = glfwCreateWindow(960, 620, "Unreal Map Draw Tool 1.16", NULL, NULL);
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
#endif