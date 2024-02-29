#include "precomp.h"
#include "terrain.h"
#include "math/lerp.h"
#include "world/biome.h"
#include "math/random.h"
#include "interface/interface.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmath>
#include <chrono>

Game* CreateGame() { return new Terrain(); }

void Terrain::Init()
{
	skyDomeLightScale = 6.0f;
	skyDomeImage = "assets/sky.hdr";

	// Restore camera, if possible
	FILE* f;

	f = fopen("camera.dat", "rb");
	if (f)
	{
		fread(&cameraDirection, 1, sizeof(cameraDirection), f);
		fread(&cameraPosition, 1, sizeof(cameraPosition), f);
		fclose(f);
	}

	f = fopen("layers.dat", "rb");
	if (f)
	{
		fread(&continentalness, 1, sizeof(continentalness), f);
		fread(&erosion, 1, sizeof(erosion), f);
		fread(&peaks, 1, sizeof(peaks), f);
		fread(&temperature, 1, sizeof(temperature), f);
		fread(&humidity, 1, sizeof(humidity), f);
		fread(&contdensity, 1, sizeof(contdensity), f);
		fread(&density, 1, sizeof(density), f);
		fread(&peakdensity, 1, sizeof(peakdensity), f);
		fclose(f);
	}

	// Load spline path
	CameraPoint p;
	FILE* fp = fopen("assets/splinepath.bin", "rb");
	while (true)
	{
		fread(&p.cameraPosition, 1, sizeof(p.cameraPosition), fp);
		size_t s = fread(&p.cameraDirection, 1, sizeof(p.cameraDirection), fp);
		if (s > 0) spline.push_back(p); else break;
	}
	fclose(fp);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(GetGlfwWindow(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();
}

void Terrain::HandleInput(float deltaTime)
{
#if 1
	if (GetAsyncKeyState(VK_RBUTTON))
	{
		glfwSetInputMode(GetGlfwWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	if (!GetAsyncKeyState(VK_RBUTTON))
	{
		glfwSetInputMode(GetGlfwWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	// Freecam controls
	if (GetAsyncKeyState(VK_RBUTTON))
	{
		float rotationSpeed = 0.1f;
		static float movementConstant = 0.01f;
		float movementSpeed = (deltaTime / 1000.0f) * movementConstant;
		static float mouseX = atan2(cameraDirection.x, cameraDirection.z) * (180.0f / PI) / rotationSpeed;
		static float mouseY = acos(cameraDirection.y) * (180.0f / PI) / rotationSpeed;

		if (mouseScroll.x || mouseScroll.y)
		{
			movementConstant *= pow(1.1f, mouseScroll.y);
		}

		mouseX += mouseDelta.x;
		mouseY = clamp(mouseY + mouseDelta.y, 1.0f, 1799.0f);

		float forward = movementSpeed * (GetAsyncKeyState('S') - GetAsyncKeyState('W'));
		float left = movementSpeed * (GetAsyncKeyState('D') - GetAsyncKeyState('A'));
		float up = movementSpeed * (GetAsyncKeyState('E') - GetAsyncKeyState('Q'));

		float pitch = rotationSpeed * mouseY * (PI / 180);
		float yaw = rotationSpeed * mouseX * (PI / 180);

		cameraDirection.x = sin(yaw) * sin(pitch);
		cameraDirection.y = cos(pitch);
		cameraDirection.z = cos(yaw) * sin(pitch);
		cameraDirection = normalize(cameraDirection);

		float3 L = cross(cameraDirection, make_float3(0.0f, 1.0f, 0.0f));
		float3 U = cross(cameraDirection, L);
		cameraPosition += cameraDirection * forward + L * left + U * up;
	}

	// Enable to set spline path points, P key
	static bool pdown = false;
	static FILE* pf = 0;
	if (!GetAsyncKeyState('P')) pdown = false; else
	{
		// Save a point for the spline
		if (!pdown)
		{
			if (!pf) pf = fopen("spline.bin", "wb");
			fwrite(&cameraPosition, 1, sizeof(cameraPosition), pf);
			float3 t = cameraPosition + cameraDirection;
			fwrite(&t, 1, sizeof(t), pf);
		}
		pdown = true;
	}

	LookAt(cameraPosition, cameraPosition + cameraDirection);
#else
	// Playback of recorded spline path
	// TODO: add this to imgui
	const size_t N = spline.size();
	CameraPoint p0 = spline[(splineIndex + (N - 1)) % N], p1 = spline[splineIndex];
	CameraPoint p2 = spline[(splineIndex + 1) % N], p3 = spline[(splineIndex + 2) % N];
	LookAt(CatmullRom(p0.cameraPosition, p1.cameraPosition, p2.cameraPosition, p3.cameraPosition),
		   CatmullRom(p0.cameraDirection, p1.cameraDirection, p2.cameraDirection, p3.cameraDirection));
	if ((splineLerp += deltaTime * 0.0005f) > 1) splineLerp -= 1, splineIndex = (splineIndex + 1) % N;
#endif

	mouseDelta = { 0, 0 };
	mouseScroll = { 0, 0 };
}

void Terrain::HandleInterface()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Text("Voxels (%.2f mv)		Delay (%lld ms)", voxels / 1000000.0f, delay);

	parameters.dirty |= ImGui::RadioButton("2D", &parameters.dimension, 0);
	ImGui::SameLine();
	parameters.dirty |= ImGui::RadioButton("3D", &parameters.dimension, 1);

	parameters.dirty |= ImGui::Checkbox("Color blend", &parameters.blend);
	parameters.dirty |= ImGui::Checkbox("Water fill", &parameters.waterFill);
	parameters.dirty |= ImGui::Checkbox("Water erosion", &parameters.waterErosion);
	parameters.dirty |= ImGui::Checkbox("Cave inverted", &parameters.caveInverted);

	std::vector<const char*> items =
	{
		"None", "Grassland", "Desert", "Ocean"
	};

	parameters.dirty |= ImGui::Combo("Preset",
		&parameters.presetIndex, items.data(),
			static_cast<int>(items.size()));
	switch (parameters.presetIndex)
	{
		FILE* f;

		case 1:
		{
			f = fopen("layer_grassland.dat", "rb");

			if (f)
			{
				fread(&continentalness, 1, sizeof(continentalness), f);
				fread(&erosion, 1, sizeof(erosion), f);
				fread(&peaks, 1, sizeof(peaks), f);
				fread(&temperature, 1, sizeof(temperature), f);
				fread(&humidity, 1, sizeof(humidity), f);
				fread(&contdensity, 1, sizeof(contdensity), f);
				fread(&density, 1, sizeof(density), f);
				fread(&peakdensity, 1, sizeof(peakdensity), f);
				fclose(f);
			}

			parameters.dirty = true;
			break;
		}

		case 2:
		{
			f = fopen("layer_desert.dat", "rb");

			if (f)
			{
				fread(&continentalness, 1, sizeof(continentalness), f);
				fread(&erosion, 1, sizeof(erosion), f);
				fread(&peaks, 1, sizeof(peaks), f);
				fread(&temperature, 1, sizeof(temperature), f);
				fread(&humidity, 1, sizeof(humidity), f);
				fread(&contdensity, 1, sizeof(contdensity), f);
				fread(&density, 1, sizeof(density), f);
				fread(&peakdensity, 1, sizeof(peakdensity), f);
				fclose(f);
			}

			parameters.dirty = true;
			break;
		}

		case 3:
		{
			f = fopen("layer_ocean.dat", "rb");

			if (f)
			{
				fread(&continentalness, 1, sizeof(continentalness), f);
				fread(&erosion, 1, sizeof(erosion), f);
				fread(&peaks, 1, sizeof(peaks), f);
				fread(&temperature, 1, sizeof(temperature), f);
				fread(&humidity, 1, sizeof(humidity), f);
				fread(&contdensity, 1, sizeof(contdensity), f);
				fread(&density, 1, sizeof(density), f);
				fread(&peakdensity, 1, sizeof(peakdensity), f);
				fclose(f);
			}

			parameters.dirty = true;
			break;
		}
	}

	items =
	{
		"All", "Continentalness", "Erosion", "Peaks", "Humidity",
		"Density Continental", "Density", "Density Peaks"
	};

	parameters.dirty |= ImGui::Combo("Layer",
		&parameters.layerIndex, items.data(),
			static_cast<int>(items.size()));
	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");
	parameters.dirty |= ParameterSliderInt("Terrain X", parameters.terrainScaleX, 0, 1024);
	parameters.dirty |= ParameterSliderInt("Terrain Z", parameters.terrainScaleZ, 0, 1024);
	parameters.dirty |= ParameterSliderInt("Terrain Offset X", parameters.terrainOffsetX, -8192, 8192);
	parameters.dirty |= ParameterSliderInt("Terrain Offset Z", parameters.terrainOffsetZ, -8192, 8192);

	if (ImGui::TreeNode("Elevation"))
	{
		if (ImGui::TreeNode("Continental"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(continentalness);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Erosion"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(erosion);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Simulated"))
			{
				parameters.dirty |= ImGui::SliderInt("Iterations", &parameters.erosionIterations, 0, 131070);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Peaks"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(peaks);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Climate"))
	{
		if (ImGui::TreeNode("Humidity"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(humidity);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Density"))
	{
		if (ImGui::TreeNode("Continental"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(contdensity);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Density"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(density);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Peaks"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				parameters.dirty |= LayerParameter(peakdensity);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void static Generate(const Columns* world, const Layer& contdensity, const Layer& density, const Layer& peakdensity, const Parameters& parameters, const int thread)
{
	int section = parameters.terrainScaleX / THREAD_LIMIT;
	int start = thread * section;
	int end = start + section;

	for (int x = start; x < end; x++)
	{
		for (int z = 0; z < parameters.terrainScaleZ; z++)
		{
			const Column& column = (*world)[x][z];
			const uint8_t level = column.level;

			const uint16_t water = (0x006 + (static_cast<int>(0x006 * level / 60.0f) << 4));
			uint16_t color = (level < 61 && parameters.dimension) ? water : colors[column.biome];

			if (parameters.blend)
			{
				uint16_t nearby = LerpColors
				(
					LerpColors(colors[(*world)[max(x - 4, 0)][z].biome], colors[(*world)[min(x + 4, 1023)][z].biome], 0.5f),
					LerpColors(colors[(*world)[x][max(z - 4, 0)].biome], colors[(*world)[x][min(z + 4, 1023)].biome], 0.5f),
					0.5f
				);

				color = LerpColors(nearby, color, 0.5f);
			}

			if (parameters.layerIndex)
			{
				int f = max(static_cast<int>(0x00f * level / 60.0f), 0x001);
				color = (f << 8) | (f << 4) | f;
			}

			const float fx = static_cast<float>(x + 0),
				fz = static_cast<float>(z + 0);
			float contdensityNoise =
				LerpPoints(contdensity, (contdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * contdensity.noise.GetNoise(fx, fz);
			float peakdensityNoise =
				LerpPoints(peakdensity, (peakdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peakdensity.noise.GetNoise(fx, fz);

			if (parameters.waterFill&& level < 61)
			{
				for (int y = 60; y > parameters.dimension ? level : 0; y--)
				{
					Plot(x, y, z, color);
				}
			}

			for (int y = parameters.dimension ? level : 0; y > -1; y--)
			{
				float fy = static_cast<float>(y);

				bool bounds = y < 40 + peakdensityNoise * 4.0f &&
					y > 36 + peakdensityNoise * 4.0f;
				bool noodle = abs(contdensityNoise * 10.0f +
					density.noise.GetNoise(fx, fy, fz) * 5.0f +
					peakdensity.noise.GetNoise(fx, fy, fz)) < 0.5f;

				/*if (parameters.dimension && y < level - 1)
				{
					color = colors[13];
				}

				if (parameters.dimension && y < level - (density.noise.GetNoise(fx, fz) + 1.0f) * 8.0f)
				{
					color = colors[14];
				}*/

				if (level > 60 && bounds && noodle)
				{
					if (parameters.caveInverted)
					{
						Plot(x, y, z, color);
					}

					continue;
				}

				if (!parameters.caveInverted)
				{
					Plot(x, y, z, color);
				}
			}
		}
	}
}

void Terrain::Tick(float deltaTime)
{
	static size_t ticks = 0;
	HandleInput(deltaTime);

	const std::array<Layer, 7> layers =
	{
		continentalness,
		erosion,
		peaks,
		humidity,
		contdensity,
		density,
		peakdensity
	};

	// Gather noise data
	if (parameters.dirty)
	{
		SetParameters(continentalness);
		SetParameters(erosion);
		SetParameters(peaks);
		SetParameters(humidity);
		SetParameters(contdensity);
		SetParameters(density);
		SetParameters(peakdensity);

		voxels = 0;
		ClearWorld();
		auto start = std::chrono::system_clock::now();

		// Height and biome type in a 2d array.
		Columns* world = new Columns;

		for (int x = 0; x < 1024; x++)
		{
			for (int z = 0; z < 1024; z++)
			{
				float fx = static_cast<float>(x + parameters.terrainOffsetX),
					fz = static_cast<float>(z + parameters.terrainOffsetZ);

				float continentalnessNoise =
					LerpPoints(continentalness, (continentalness.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * continentalness.noise.GetNoise(fx, fz);
				float erosionNoise =
					LerpPoints(erosion, (erosion.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * erosion.noise.GetNoise(fx, fz);
				float peaksNoise =
					LerpPoints(peaks, (peaks.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peaks.noise.GetNoise(fx, fz);

				float elevationNoise = clamp(((continentalnessNoise * 200.0f +
					(peaksNoise + 0.3f) * 40.0f) * erosionNoise + 120.0f) / 2.0f, 0.0f, 240.0f);
				float humidityNoise =
					0.1f * powf(2, -10.0f * powf(x / 512.0f - 1.0f, 2.0f)) +
					humidity.noise.GetNoise(fx, fz);

				const uint8_t biome = BiomeFunction(elevationNoise / 60.0f - 1.0f, humidityNoise);

				uint8_t level = static_cast<uint8_t>(elevationNoise);
				level = parameters.layerIndex ?
					static_cast<uint8_t>((layers[parameters.layerIndex - 1].
						noise.GetNoise(fx, fz) + 1.0f) * 30.0f) : level;

				// Not entirely accurate, but way easier.
				voxels += level;

				(*world)[x][z] =
				{
					level,
					biome
				};
			}
		}

		for (int iteration = 0; iteration < parameters.erosionIterations; iteration++)
		{
			// Random droplet location
			int rx = urandom() % parameters.terrainScaleX,
				rz = urandom() % parameters.terrainScaleZ;
			int lx = 0, lz = 0;
			std::vector<std::pair<int, int>> steps;

			for (int step = 0; step < 128; step++)
			{
				uint8_t lowest = 255;

				for (int x = -1; x < 2; x++)
				{
					if (rx + x < 0 || rx + x > parameters.terrainScaleX - 1)
					{
						continue;
					}

					for (int z = -1; z < 2; z++)
					{
						if (x == 0 && z == 0)
						{
							continue;
						}

						if (rz + z < 0 || rz + z > parameters.terrainScaleZ - 1)
						{
							continue;
						}

						const uint8_t level = (*world)[rx + x][rz + z].level;

						if (level < lowest)
						{
							lowest = level;
							lx = x;
							lz = z;
						}
					}
				}

				rx += lx; rz += lz;

				if (rx < 0 || rx > parameters.terrainScaleX - 1 ||
					rz < 0 || rz > parameters.terrainScaleZ - 1)
				{
					break;
				}

				steps.emplace_back(rx, rz);

				//(*world)[rx][rz].level++;
				//(*world)[rx][rz].biome = 15;
			}

			for (auto step : steps)
			{
				//int3 p = { step.first, 255, step.second };
				//Plot(p, 0xaaa);
				(*world)[step.first][step.second].biome = 15;
			}

			// int3 s = { steps[0].first, 255, steps[0].second };
			// int3 e = { steps[steps.size() - 1].first, 255, steps[steps.size() - 1].second};
			// Plot(s, 0x00f);
			// Plot(e, 0xf00);
		}

		/*for (int iteration = 0; iteration < parameters.erosionIterations; iteration++)
		{
		next:
			constexpr float scale = 10.0f;

			// Start location
			float rx = urandom() % parameters.terrainScaleX,
				rz = urandom() % parameters.terrainScaleZ;
			
			//float vx = (random() % 100) / 1000.0f,
			//	vz = (random() % 100) / 1000.0f;
			float vx = 0.0f, vz = 0.0f;
			float stolen = 0;

			for (int step = 0; step < 128; step++)
			{
				for (int x = -1; x < 2; x++)
				{
					for (int z = -1; z < 2; z++)
					{
						// Out-of-bound check
						if (((int)rx + x) < 0 || ((int)rx + x) > parameters.terrainScaleX - 1 ||
							((int)rz + z) < 0 || ((int)rz + z) > parameters.terrainScaleX - 1)
						{
							goto next;
						}

						vx += x * (255 - (*world)[rx + x][rz + z].level) / 255.0f;
						vz += z * (255 - (*world)[rx + x][rz + z].level) / 255.0f;
					}
				}

				// Out-of-bound check
				if ((int)(rx + vx * scale) < 0 || (int)(rx + vx * scale) > parameters.terrainScaleX - 1 ||
					(int)(rz + vz * scale) < 0 || (int)(rz + vz * scale) > parameters.terrainScaleX - 1)
				{
					goto next;
				}

				int delta = (*world)[(int)(rx + vx * scale)][(int)(rz + vz * scale)].level - (*world)[(int)rx][(int)rz].level;

				if (delta < 0 && stolen < 20.0f)
				{
					// float f = delta / 10.0f;
					// (*world)[(int)rx][(int)rz].erosion += f;
					// stolen += abs(f);

					if ((*world)[(int)rx][(int)rz].level > (*world)[(int)(rx + vx * scale)][(int)(rz + vz * scale)].level)
					{
						stolen++;
						(*world)[(int)rx][(int)rz].level--;
					}
				}

				// Take step, move with velocity.
				rx += vx * scale; rz += vz * scale;

				if (delta > 0 || stolen > 10.0f)
				{
					// float f = delta / 10.0f;
					// (*world)[(int)rx][(int)rz].erosion += f;
					// stolen = max(stolen - f, 0.0f);

					stolen--;
					(*world)[(int)rx][(int)rz].level++;
				}
			}
		}*/


#ifdef MULTI_THREADING
		std::vector<std::thread> threads;

		while (threads.size() < THREAD_LIMIT)
		{
			threads.emplace_back(Generate, world, contdensity, density, peakdensity,
				parameters, static_cast<int>(threads.size()));
		}

		for (auto& thread : threads)
		{
			thread.join();
		}
#else
		for (int thread = 0; thread < THREAD_LIMIT; thread++)
		{
			// Threads are not used here,
			// they are faked so the function can be re-used.
			Generate(world, contdensity, density, peakdensity, parameters, thread);
		}
#endif

		delete world;

		auto end = std::chrono::system_clock::now();
		auto elapsed = duration_cast
			<std::chrono::milliseconds>(end - start);
		delay = elapsed.count();
		parameters.dirty = false;
	}

	ticks++;
}

void Terrain::Predraw()
{
	// Empty
}

void Terrain::Postdraw()
{
	HandleInterface();
}

void Terrain::Shutdown()
{
	FILE* f;

	f = fopen("layers.dat", "wb");
	fwrite(&continentalness, 1, sizeof(continentalness), f);
	fwrite(&erosion, 1, sizeof(erosion), f);
	fwrite(&peaks, 1, sizeof(peaks), f);
	fwrite(&temperature, 1, sizeof(temperature), f);
	fwrite(&humidity, 1, sizeof(humidity), f);
	fwrite(&contdensity, 1, sizeof(contdensity), f);
	fwrite(&density, 1, sizeof(density), f);
	fwrite(&peakdensity, 1, sizeof(peakdensity), f);
	fclose(f);

	f = fopen("camera.dat", "wb");
	fwrite(&cameraDirection, 1, sizeof(cameraDirection), f);
	fwrite(&cameraPosition, 1, sizeof(cameraPosition), f);
	fclose(f);

	// uint sprite = CreateSprite(0, 0, 0, 256, 256, 256);
	// SaveSprite(sprite, "world.vox");
}