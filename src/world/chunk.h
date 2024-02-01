#pragma once

#include "layer.h"

#include <array>
#include <vector>
#include <stdint.h>

constexpr size_t CHUNK_WIDTH = 64;
constexpr size_t CHUNK_HEIGHT = 256;

struct Column
{
	int8_t limit, color;
	std::array<float, 9> noise;
	std::array<int16_t, CHUNK_HEIGHT> rows;
};

struct Chunk
{
	int8_t chunkX, chunkZ;
	std::array<Column, CHUNK_WIDTH * CHUNK_WIDTH> columns;

	Chunk(int8_t chunkX, int8_t chunkZ) : chunkX(chunkX), chunkZ(chunkZ)
	{
		// Empty
	}
};

void ChunkGenerateNoise(Chunk& chunk, const Layer& continentalness, const Layer& erosion, const Layer& peaks,
	const Layer& temperature, const Layer& humidity, const Layer& contdensity, const Layer& density, const Layer& peakdensity, int ox, int oz);
