#include "chunk.h"
#include "layer.h"
#include "src/math/clamp.h"

void ChunkGenerateNoise(Chunk& chunk, const Layer& continentalness, const Layer& erosion, const Layer& peaks,
	const Layer& temperature, const Layer& humidity, const Layer& contdensity, const Layer& density, const Layer& peakdensity, int ox, int oz)
{
	/*for (size_t x = 0; x < CHUNK_WIDTH; x++)
	{
		for (size_t z = 0; z < CHUNK_WIDTH; z++)
		{
			float fx = static_cast<float>(x + ox),
				fz = static_cast<float>(z + oz);

			float continentalnessNoise =
				LerpPoints(continentalness, (continentalness.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * continentalness.noise.GetNoise(fx, fz);
			float erosionNoise =
				LerpPoints(erosion, (erosion.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * erosion.noise.GetNoise(fx, fz);
			float peaksNoise =
				LerpPoints(peaks, (peaks.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peaks.noise.GetNoise(fx, fz);

			float elevationNoise = clamp(((continentalnessNoise * 200.0f +
										 (peaksNoise + 0.3f) * 40.0f) * erosionNoise + 120.0f) / 2.0f, 0.0f, 240.0f);
			float temperatureNoise =
				temperature.noise.GetNoise(fx, fz);
			float humidityNoise =
				0.1f * powf(2, -10.0f * powf(x / 512.0f - 1.0f, 2.0f)) +
				humidity.noise.GetNoise(fx, fz);

			float contdensityNoise =
				LerpPoints(contdensity, (contdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * contdensity.noise.GetNoise(fx, fz);
			float densityNoise =
				LerpPoints(density, (density.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * density.noise.GetNoise(fx, fz);
			float peakdensityNoise =
				LerpPoints(peakdensity, (peakdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peakdensity.noise.GetNoise(fx, fz);

			chunk.columns[z + x * CHUNK_WIDTH].noise =
			{
				continentalnessNoise,
				erosionNoise,
				peaksNoise,
				elevationNoise,
				temperatureNoise,
				humidityNoise,
				contdensityNoise,
				densityNoise,
				peakdensityNoise
			};
		}
	}*/
}