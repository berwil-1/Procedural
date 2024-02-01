#include "precomp.h"
#include "terrain.h"
#include "math/lerp.h"
#include "world/chunk.h"
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

#if 1
	continentalness.points =
	{
		1.0f,
		0.3f,
		0.05f,
		0.05f,
		0.05f,
		0.07f,
		0.5f,
		0.5f,
		0.5f,
		0.9f,
		0.91f,
		0.92f,
		0.92f,
		0.92f,
		0.93f,
		0.94f,
		0.96f,
		0.98f,
		0.99f,
		1.0f
	};

	erosion.points =
	{
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f

		/*1.0f,
		0.8f,
		0.7f,
		0.65f,
		0.64f,
		0.6f,
		0.58f,
		0.6f,
		0.66f,
		0.42f,
		0.3f,
		0.22f,
		0.22f,
		0.21f,
		0.20f,
		0.4f,
		0.5f,
		0.5f,
		0.2f,
		0.1f*/
	};

	peaks.points =
	{
		/*0.1f,
		0.15f,
		0.2f,
		0.27f,
		0.29f,
		0.3f,
		0.33f,
		0.36f,
		0.44f,
		0.5f,
		0.59f,
		0.64f,
		0.69f,
		0.73f,
		0.76f,
		0.8f,
		0.87f,
		0.96f,
		0.99f,
		1.0f*/

		1.0f,
		0.95f,
		0.9f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.5f,
		0.59f,
		0.64f,
		0.69f,
		0.73f,
		0.76f,
		0.8f,
		0.87f,
		0.96f,
		0.99f,
		1.0f
	};

	contdensity.points =
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
#endif
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
	const size_t N = splinePath.size();
	PathPoint p0 = splinePath[(pathPt + (N - 1)) % N], p1 = splinePath[pathPt];
	PathPoint p2 = splinePath[(pathPt + 1) % N], p3 = splinePath[(pathPt + 2) % N];
	LookAt(CatmullRom(p0.O, p1.O, p2.O, p3.O), CatmullRom(p0.D, p1.D, p2.D, p3.D));
	if ((t += deltaTime * 0.0005f) > 1) t -= 1, pathPt = (pathPt + 1) % N;
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

	std::vector<const char*> items;
	items = { "All", "Continentalness", "Erosion", "Peaks", "Temperature", "Humidity" };
	dirty |= ImGui::Combo("Preset", &presetTest, items.data(),
		static_cast<int>(items.size()));

	items = { "None" };
	for (const Biome& biome : biomes) items.push_back(biome.name);
	dirty |= ImGui::Combo("Palette", &paletteTest, items.data(),
		static_cast<int>(items.size()));

	dirty |= ImGui::RadioButton("2D", &dimension, 0);
	ImGui::SameLine();
	dirty |= ImGui::RadioButton("3D", &dimension, 1);

	dirty |= ImGui::Checkbox("Color blend", &colorBlend);
	dirty |= ImGui::Checkbox("Water fill", &waterFill);
	dirty |= ImGui::Checkbox("Water erosion", &waterErosion);
	dirty |= ImGui::Checkbox("Cave inverted", &caveInverted);
	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");
	dirty |= ParameterSliderInt("Terrain X", terrainX, 0, 1024);
	dirty |= ParameterSliderInt("Terrain Z", terrainZ, 0, 1024);
	dirty |= ParameterSliderInt("Terrain Offset X", terrainOffsetX, -8192, 8192);
	dirty |= ParameterSliderInt("Terrain Offset Z", terrainOffsetZ, -8192, 8192);

	auto TreeNodeWithLayerParameter = [this](const char* category, const char* subcategory, Layer& parameter) -> bool
	{
		if (ImGui::TreeNode(category))
		{
			if (ImGui::TreeNode(subcategory))
			{
				if (ImGui::TreeNode("Noise"))
				{
					dirty |= LayerParameter(parameter);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
			return true;
		}
		return false;
	};

	TreeNodeWithLayerParameter("Elevation", "Continental", continentalness);
	TreeNodeWithLayerParameter("Elevation", "Erosion", erosion);
	TreeNodeWithLayerParameter("Elevation", "Peaks", peaks);

	TreeNodeWithLayerParameter("Climate", "Temperature", temperature);
	TreeNodeWithLayerParameter("Climate", "Humidity", humidity);

	TreeNodeWithLayerParameter("Density", "Continental", contdensity);
	TreeNodeWithLayerParameter("Density", "Density", density);
	TreeNodeWithLayerParameter("Density", "Peaks", peakdensity);


	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Terrain::Tick(float deltaTime)
{
	static size_t ticks = 0;
	HandleInput(deltaTime);

	// Gather noise data
	if (dirty)
	{
		SetParameters(continentalness);
		SetParameters(erosion);
		SetParameters(peaks);
		SetParameters(temperature);
		SetParameters(humidity);
		SetParameters(contdensity);
		SetParameters(density);
		SetParameters(peakdensity);

		voxels = 0;
		ClearWorld();
		auto start = std::chrono::system_clock::now();
	
		std::vector<Chunk> chunks;
		chunks.reserve(4096);

		for (size_t i = 0; i < 4096; i++)
		{
			chunks.emplace_back(static_cast<int8_t>(i / 64), static_cast<int8_t>(i % 64));
		}

		for (Chunk& chunk : chunks)
		{
			for (size_t x = 0; x < 64; x++)
			{
				for (size_t z = 0; z < 64; z++)
				{
					float fx = static_cast<float>(x + terrainOffsetX),
						fz = static_cast<float>(z + terrainOffsetZ);

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
			}

			//ChunkGenerateNoise(chunk, continentalness, erosion, peaks, temperature, humidity, contdensity, density, peakdensity, terrainOffsetX, terrainOffsetZ);
		}



		// TODO: make 2d array of column struct, each colum contains its biome type and limit
		//std::array<std::array<std::pair<int, uint16_t>,
		//	1024>, 1024>* world = new std::array<std::array<std::pair<int, uint16_t>, 1024>, 1024>();

		// TODO: move lambda functions into actual header files
		// TODO: don't store pair<limit, color> store the limit and biome id compressed into int16.
		// TODO: multithread the execution
		// TODO: make a voxel at coord function



		/*
		for (int x = 0; x < 1024; x++)
		{
			for (int z = 0; z < 1024; z++)
			{
				float fx = static_cast<float>(x + terrainOffsetX),
					fz = static_cast<float>(z + terrainOffsetZ);

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

				const uint8_t biome = BiomeFunction(elevationNoise / 60.0f - 1.0f,
													temperatureNoise, humidityNoise);
				int limit = static_cast<int>(dimension ? elevationNoise : 1);

				if (waterFill && elevationNoise < 60.0f)
				{
					limit = 60;
				}

				(*world)[x][z] = { limit, elevationNoise < 60.0f ?
					(0x006 + (static_cast<int>(0x006 * elevationNoise / 60.0f) << 4)) : colors[biome]};
			}
		}

		for (int x = 0; x < terrainX; x++)
		{
			for (int z = 0; z < terrainZ; z++)
			{
				int limit = (*world)[x][z].first;

				if (waterErosion && limit < 60)
				{
					int a = static_cast<int>(lerp((*world)[max(x - 4, 0)][z].first, (*world)[min(x + 4, 1023)][z].first, 0.5f));
					int b = static_cast<int>(lerp((*world)[x][max(z - 4, 0)].first, (*world)[x][min(z + 4, 1023)].first, 0.5f));
					limit = static_cast<int>(lerp(a, b, 0.5f));
				}

				const uint16_t color =
					colorBlend ?
					LerpColors
					(
						LerpColors((*world)[max(x - 4, 0)][z].second, (*world)[min(x + 4, 1023)][z].second, 0.5f),
						LerpColors((*world)[x][max(z - 4, 0)].second, (*world)[x][min(z + 4, 1023)].second, 0.5f),
						0.5f
					) : (*world)[x][z].second;

				for (int y = limit; y > -1; y--)
				{
					float fx = static_cast<float>(x + terrainOffsetX),
						fy = static_cast<float>(y), fz = static_cast<float>(z + terrainOffsetZ);
				// TODO: move noise out?
					float contdensityNoise =
						LerpPoints(contdensity, (contdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * contdensity.noise.GetNoise(fx, fz);
					float densityNoise =
						LerpPoints(density, (density.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * density.noise.GetNoise(fx, fz);
					float peakdensityNoise =
						LerpPoints(peakdensity, (peakdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peakdensity.noise.GetNoise(fx, fz);

					bool bounds = y < 40 + peakdensity.noise.GetNoise(fx, fz) * 4.0f &&
						y > 36 + peakdensity.noise.GetNoise(fx + 1.0f, fz + 1.0f) * 4.0f;
					bool noodle = abs(contdensity.noise.GetNoise(fx, fz) * 10.0f +
						density.noise.GetNoise(fx, fy, fz) * 5.0f +
						peakdensity.noise.GetNoise(fx, fy, fz)) < 0.5f;
					
					if (bounds && noodle)
					{
						if (caveInverted)
						{
							Plot(x, y, z, color);
							voxels++;
						}

						continue;
					}

					if (!caveInverted)
					{
						Plot(x, y, z, color);
						voxels++;
					}
				}
			}
		}
		*/

		//delete world;

		using namespace std::chrono;
		auto end = system_clock::now();
		auto elapsed = duration_cast<milliseconds>(end - start);
		delay = elapsed.count();
		dirty = false;
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