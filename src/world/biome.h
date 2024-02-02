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

constexpr std::array<uint16_t, 15> colors =
{
	/*0x553,
	0x333,
	0x777,
	0x453,
	0x473,
	0x664,
	0x242,
	0x342,
	0x452,
	0x132,
	0x243,
	0x654,
	0x124*/

	0x553,							// 0
	0x333,							// 1
	0x777,							// 2
	0x453,							// 3
	0x463,							// 4
	0x664,							// 5
	0x262,							// 6
	0x362,							// 7
	0x462,							// 8
	0x162,							// 9
	0x263,							// 10
	0x654,							// 11
	0x066,							// 12
	0x321,							// 13 (dirt)
	0x555							// 14 (stone)
};

constexpr std::array<Biome, 13> biomes =
{
	"TUNDRA",						// 0
	"BARE",							// 1
	"SNOW",							// 2
	"TAIGA",						// 3
	"SHRUBLAND",					// 4
	"TEMPERATE_DESERT",				// 5
	"TEMPERATE_RAIN_FOREST",		// 6
	"TEMPERATE_DECIDUOUS_FOREST",	// 7
	"GRASSLAND",					// 8
	"TROPICAL_RAIN_FOREST",			// 9
	"TROPICAL_SEASONAL_FOREST",		// 10
	"SUBTROPICAL_DESERT",			// 11
	"OCEAN"							// 12
};

uint8_t BiomeFunction(const float elevation, const float humidity);