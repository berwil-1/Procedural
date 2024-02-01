#pragma once

#include "src/world/layer.h"

float LerpPoints(const Layer& layer, const float x)
{
	const std::array<float, 20>& points = layer.points;
	size_t lower = 0, upper = 19;

	for (size_t i = 0; i < points.size() - 1; i++)
	{
		// Is x within bounds
		if (x >= (i / 20.0f) && x < (i + 1) / 20.0f)
		{
			lower = i;
			upper = i + 1;
			break;
		}
	}

	float t = (x - (lower / 20.0f)) / ((upper / 20.0f) - (lower / 20.0f));
	return std::lerp(points[lower], points[upper], t);
}

uint16_t LerpColors(const uint16_t a, const uint16_t b, const float t)
{
	uint16_t ra = (a >> 8);
	uint16_t rb = (b >> 8);
	uint16_t rl = static_cast<uint16_t>(std::lerp(ra, rb, t));

	uint16_t ga = (a >> 4) & 15;
	uint16_t gb = (b >> 4) & 15;
	uint16_t gl = static_cast<uint16_t>(std::lerp(ga, gb, t));

	uint16_t ba = a & 15;
	uint16_t bb = b & 15;
	uint16_t bl = static_cast<uint16_t>(std::lerp(ba, bb, t));

	return (rl << 8) | (gl << 4) | bl;
}