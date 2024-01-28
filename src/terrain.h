#pragma once
#include "FastNoiseLite.h"
#include "lib/imgui/imgui.h"

namespace Tmpl8
{

	struct CameraPoint
	{
		float3 cameraPosition, cameraDirection;
	};

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

	class Terrain : public Game
	{
	public:

		// Game flow methods
		void Init();
		void HandleInput(float deltaTime);
		void HandleInterface();
		void Tick(float deltaTime);
		void Predraw();
		void Postdraw();
		void Shutdown();

		// Input handling
		void MouseUp(int button) { /* implement if you want to detect mouse button presses */ }
		void MouseDown(int button) { /* implement if you want to detect mouse button presses */ }
		void MouseMove(int x, int y) { mouseDelta.x = (x - mousePos.x); mouseDelta.y = (y - mousePos.y); mousePos.x = x, mousePos.y = y; }
		void KeyUp(int key) { /* implement if you want to handle keys */ }
		void KeyDown(int key) { /* implement if you want to handle keys */ }
		float3 CatmullRom(const float3& p0, const float3& p1, const float3& p2, const float3& p3)
		{
			const float3 c = 2 * p0 - 5 * p1 + 4 * p2 - p3, d = 3 * (p1 - p2) + p3 - p0;
			return 0.5f * (2 * p1 + ((p2 - p0) * splineLerp) + (c * splineLerp * splineLerp) + (d * splineLerp * splineLerp * splineLerp));
		}

		// Camera
		float3 cameraDirection = make_float3(0.0f, 0.0f, 1.0f);
		float3 cameraPosition = make_float3(0.0f, 0.0f, 0.0f);

		// Mouse
		int2 mousePos = {0, 0};
		int2 mouseDelta = {0, 0};

		// Spline path data
		int splineIndex = 1;
		float splineLerp = 0;
		vector<CameraPoint> spline;

		// Other data
		int voxels = 0;
		long long delay = 0;

		// Terrain
		bool dirty = true;
		int dimension = 0; // 0 = 2d, 1 = 3d
		int presetTest = 0, paletteTest = 0;
		int terrainX = 512, terrainZ = 512, terrainOffsetX = 0, terrainOffsetZ = 0;

		Layer continentalness, erosion, peaks,
			temperature, humidity;
	};

} // namespace Tmpl8