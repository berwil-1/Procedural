#pragma once

#include <array>
#include <vector>

// Grass
constexpr std::array<uint, 8> PALETTE_GRASS =
{
	0x340, 0x680, 0x650, 0xee0, 0xee0, 0xab0, 0x570, 0x590
};
constexpr std::array<uint, 8> PALETTE_GRASS_SNOW =
{
	0x332, 0x8a6, 0xbc8, 0xeee, 0x9a7, 0x895, 0x673, 0x995
};
constexpr std::array<uint, 8> PALETTE_GRASS_DARK =
{
	0x330, 0x570, 0x440, 0x430, 0x550, 0x340, 0x330, 0x220
};

// Dirt
constexpr std::array<uint, 8> PALETTE_DIRT =
{
	0x321, 0x321, 0x321, 0x321, 0x321, 0x321, 0x321, 0x321
};
constexpr std::array<uint, 8> PALETTE_DIRT_MUD =
{
	0x210, 0x210, 0x210, 0x210, 0x210, 0x210, 0x210, 0x210
};
constexpr std::array<uint, 8> PALETTE_DIRT_PODZOL =
{
	0x321, 0x210, 0x432, 0x321, 0x321, 0x210, 0x321, 0x321
};
constexpr std::array<uint, 8> PALETTE_DIRT_STONE =
{
	0x321, 0x444, 0x321, 0x321, 0x332, 0x321, 0x321, 0x321
};

// Sand
constexpr std::array<uint, 8> PALETTE_SAND =
{
	0xec4, 0xb93, 0x641, 0xa72, 0xca4, 0x852, 0xa73, 0x751
};

// Stone
constexpr std::array<uint, 8> PALETTE_STONE =
{
	0x554, 0x444, 0x666, 0x665, 0x332, 0x443, 0x555, 0x333
};
constexpr std::array<uint, 8> PALETTE_STONE_SNOW =
{
	0xaa9, 0x777, 0xccc, 0xbbb, 0x665, 0x887, 0xaaa, 0x999
};

// Snow
constexpr std::array<uint, 8> PALETTE_SNOW =
{
	0xfff, 0xeee, 0x556, 0x889, 0x334, 0x9ab, 0xabb, 0x666
};

// Water
constexpr std::array<uint, 8> PALETTE_WATER =
{
	0x024, 0x024, 0x024, 0x024, 0x024, 0x024, 0x024, 0x024
};
constexpr std::array<uint, 8> PALETTE_WATER_ICE =
{
	0xfff, 0xeff, 0x39c, 0xcef, 0x9df, 0x013, 0x024, 0x59c
};

struct Biome
{
	float
		elevationMin, elevationMax,
		temperatureMin, temperatureMax,
		humidityMin, humidityMax;

	std::array<std::array<uint, 8>, 4> surface;
	std::array<std::array<uint, 8>, 4> layers;
	std::array<std::array<uint, 8>, 2> liquids;
};

constexpr Biome BIOME_FOREST =
{
	// Elevation (min/max)
	0.0f, 0.4f,
	
	// Temperature (min/max)
	-0.2, 0.4,

	// Humidity (min/max)
	-0.2, 0.6,

	// Surface
	PALETTE_GRASS,
	PALETTE_GRASS,
	PALETTE_GRASS,
	PALETTE_GRASS,

	// Layers
	PALETTE_DIRT,
	PALETTE_DIRT,
	PALETTE_STONE,
	PALETTE_STONE,

	// Liquids
	PALETTE_WATER,
	PALETTE_WATER
};

constexpr Biome BIOME_DESERT =
{
	// Elevation (min/max)
	-0.2f, 0.2f,

	// Temperature (min/max)
	-0.3, 1.0,

	// Humidity (min/max)
	-1.0, -0.6,

	// Surface
	PALETTE_SAND,
	PALETTE_SAND,
	PALETTE_SAND,
	PALETTE_SAND,

	// Layers
	PALETTE_SAND,
	PALETTE_SAND,
	PALETTE_DIRT,
	PALETTE_STONE,

	// Liquids
	PALETTE_WATER,
	PALETTE_WATER
};

constexpr Biome BIOME_MOUNTAIN =
{
	// Elevation (min/max)
	0.4f, 0.8f,

	// Temperature (min/max)
	-1.0, 1.0,

	// Humidity (min/max)
	-1.0, -0.4,

	// Surface
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,

	// Layers
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,

	// Liquids
	PALETTE_WATER,
	PALETTE_WATER
};

constexpr Biome BIOME_MOUNTAIN_SNOW =
{
	// Elevation (min/max)
	0.8f, 1.0f,

	// Temperature (min/max)
	-1.0, 0.0,

	// Humidity (min/max)
	-1.0, -1.0,

	// Surface
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,

	// Layers
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,
	PALETTE_STONE,

	// Liquids
	PALETTE_WATER,
	PALETTE_WATER
};