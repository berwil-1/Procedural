#pragma once

#include "FastNoiseLite.h"
#include <array>

struct Layer
{
	int
		seed = 1337,
		noiseIndex = 0,
		rotationIndex = 0,
		fractalIndex = 0,
		fractalOctaves = 3,
		distanceIndex = 0,
		returnIndex = 0,
		domainIndex = 0;

	float
		frequency = 0.01f,
		fractalLacunarity = 2.0f,
		fractalGain = 0.5f,
		fractalWeightedStrength = 0.0f,
		fractalPingPongStrength = 2.0f,
		cellularJitter = 1.0f,
		domainAmplitude = 1.0f;


	std::array<float, 20> points =
	{
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f
	};

	FastNoiseLite noise;
};