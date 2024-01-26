#pragma once

#include <array>
#include <vector>
#include <cassert>
#include <cstdint>

struct Biome
{
	const char* name;

	constexpr Biome(const char* name) : name(name)
	{
		// Empty
	}
};

constexpr std::array<uint16_t, 14> colors =
{
	/*0x555,
	0x331,
	0x222,
	0x111,
	0x231,
	0x221,
	0x332,
	0x020,
	0x120,
	0x230,
	0x011,
	0x021,
	0x442,
	0x012*/

	0x777,
	0x553,
	0x333,
	0x111,
	0x463,
	0x453,
	0x664,
	0x242,
	0x342,
	0x452,
	0x132,
	0x243,
	0x654,
	0x012
};

constexpr std::array<Biome, 14> biomes =
{
	"SNOW",							// 0
	"TUNDRA",						// 1
	"BARE",							// 2
	"SCORCHED",						// 3
	"TAIGA",						// 4
	"SHRUBLAND",					// 5
	"TEMPERATE_DESERT",				// 6
	"TEMPERATE_RAIN_FOREST",		// 7
	"TEMPERATE_DECIDUOUS_FOREST",	// 8
	"GRASSLAND",					// 9
	"TROPICAL_RAIN_FOREST",			// 10
	"TROPICAL_SEASONAL_FOREST",		// 11
	"SUBTROPICAL_DESERT",			// 12
	"OCEAN"							// 13
};

constexpr std::array<std::array<uint8_t, 6>, 4> zones =
{
	{
		{  0,  0,  0,  1, 2,  3 },
		{  4,  4,  5,  5, 6,  6 },
		{  7,  8,  8,  9, 9,  6 },
		{ 10, 10, 11, 11, 9, 12 },
	}
};

uint8_t BiomeFunction(const float elevation, const float temperature, const float humidity)
{
	if (elevation < 0.0f)
	{
		return 13; // OCEAN
	}
	
	return zones[(int)clamp(4.0f - (elevation * temperature + 1.0f) * 2.0f, 0.0f, 3.0f)]
		[(int)((humidity + 1.0f) * 3.0f)];
}