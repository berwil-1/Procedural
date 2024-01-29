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
	0x553,
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
	0x124
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

constexpr std::array<std::array<uint8_t, 6>, 4> zones =
{
	{
		{ 0, 0,  0,  1, 1,  2 },
		{ 3, 3,  4,  4, 5,  5 },
		{ 6, 6,  7,  7, 8,  8 },
		{ 9, 9, 10, 10, 11, 11 },
	}
};

uint8_t BiomeFunction(const float elevation, const float temperature, const float humidity)
{
	if (elevation < 0.0f)
	{
		return 12; // OCEAN
	}
	
	int elevationIndex = static_cast<int>(clamp(4.0f -
		elevation * 4.0f, 0.0f, 3.0f));
	int humidityIndex = static_cast<int>(clamp((humidity + 1.0f) * 3.0f - elevation, 0.0f, 5.0f));
	return zones[elevationIndex][humidityIndex];
}