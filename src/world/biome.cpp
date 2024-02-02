#include "biome.h"
#include "precomp.h"

constexpr std::array<std::array<uint8_t, 6>, 4> zones =
{
	{
		{ 0, 0,  0,  1, 1,  2 },
		{ 3, 3,  4,  4, 5,  5 },
		{ 6, 6,  7,  7, 8,  8 },
		{ 9, 9, 10, 10, 11, 11 },
	}
};

uint8_t BiomeFunction(const float elevation, const float humidity)
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
