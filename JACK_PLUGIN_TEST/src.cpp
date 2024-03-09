#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#define NTDDI_VERSION 0x05010000

#define VC_EXTRALEAN
#define _ATL_XP_TARGETING
#define WIN32_LEAN_AND_MEAN
#define PSAPI_VERSION 1


#include <Windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

const char* actionName = "About";
const char* actionDescription = "UnrealMapDrawTool is a map painting tools. Download https://github.com/UnrealKaraulov/UnrealMapDrawTool Extension of maps: .umd. ";
const char* actionDirectory = "UnrealMapDrawTool";
const char* actionFormatName = "UnrealMapDrawTool MAP";
const char* actionFormat = ".umd";

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

char cell_size[256] = "32";
char cell_height[256] = "128";
char cell_x[256] = "64";
char cell_y[256] = "64";
char cell_levels[256] = "2";
char cell_layers[256] = "3";


BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return 1;
}

BOOL CALLBACK PrintAboutForJack(HWND hwnd, LPARAM lParam) {
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);

	DWORD targetProcessId = *reinterpret_cast<DWORD*>(lParam);

	if (processId == targetProcessId) {
		MessageBoxA(hwnd, actionDescription, "UnrealMapDrawTool .umd Import Plugin by Karaulov", 0);
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
	__int64 state; // 0 is item enabled // 2 is item disabled 
	__int64 flags; // 
	void* dispatch_func;
	// UNUSED 
	__int64 reserved;
	__int64 reserved2;
};

#pragma pack(pop)

JACK_ACTION tmpJACK_ACTION{ {actionName,actionDescription,actionDirectory}, 0 , 0, (void*)dispatchFunc, 0,0 };

__int64 __fastcall vpEnumActions(void(__fastcall* jackAddAction)(JACK_ACTION*, unsigned char* qlib), unsigned char* qlib)
{
	jackAddAction(&tmpJACK_ACTION, qlib);

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

typedef __int64(__fastcall* pvpImport)(int version, const char* src, unsigned char* data);

char newSrcName[2048];

__int64 __fastcall vpImport(int formatid, const char* src, unsigned char* data)
{
	HMODULE mdl = LoadLibraryA("vpHalfLifex64.dll");
	pvpImport imp = (pvpImport)GetProcAddress(mdl,"vpImport");

	sprintf_s(newSrcName, 2048, "%s", src);

	newSrcName[strlen(src) - 1] = 'p';
	newSrcName[strlen(src) - 2] = 'a';
	newSrcName[strlen(src) - 3] = 'm';
	newSrcName[strlen(src) - 4] = '.';


	int tmp_int_value = 0;
	std::ifstream tmpmap(src, std::ios::in | std::ios::binary);
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

		tmpmap.close();
	}

	GenerateUnrealMap(newSrcName, (float)atof(cell_size), (float)atof(cell_height), (float)atof(cell_x), (float)atof(cell_y), atoi(cell_levels), atoi(cell_layers));

	__int64 retval = imp(2,newSrcName,data);
	
	DeleteFileA(newSrcName);

	return retval;
}

