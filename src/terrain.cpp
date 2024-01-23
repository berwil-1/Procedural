#include "precomp.h"
#include "terrain.h"
#include "world/biome.h"
#include "math/random.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

	if (GetAsyncKeyState(VK_RBUTTON))
	{
		float rotationSpeed = 0.1f;
		float movementSpeed =
			(deltaTime / 1000.0f) * 0.01f;
		static float mouseX =
			atan2(cameraDirection.x, cameraDirection.z) * (180.0f / PI) / rotationSpeed;
		static float mouseY =
			acos(cameraDirection.y) * (180.0f / PI) / rotationSpeed;

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
}

void Terrain::HandleInterface()
{
	constexpr std::array<const char*, 6> noiseItems =
	{
		"OpenSimplex2",
		"OpenSimplex2S",
		"Cellular",
		"Perlin",
		"ValueCubic",
		"Value"
	};

	constexpr std::array<const char*, 3> rotationItems =
	{
		"None",
		"ImproveXYPlanes",
		"ImproveXZPlanes"
	};

	constexpr std::array<const char*, 6> fractalItems =
	{
		"None",
		"FBm",
		"Ridged",
		"PingPong",
		"DomainWarpProgressive",
		"DomainWarpIndependent"
	};

	constexpr std::array<const char*, 4> distanceItems =
	{
		"Euclidean",
		"EuclideanSq",
		"Manhattan",
		"Hybrid"
	};

	constexpr std::array<const char*, 7> returnItems =
	{
		"CellValue",
		"Distance",
		"Distance2",
		"Distance2Add",
		"Distance2Sub",
		"Distance2Mul",
		"Distance2Div"
	};

	constexpr std::array<const char*, 3> domainItems =
	{
		"OpenSimplex2",
		"OpenSimplex2Reduced",
		"BasicGrid"
	};

	auto ParameterSliderInt = [this](const char* label, int& value, int min, int max)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 0.6f, 0.6f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f).Value);

		if (ImGui::Button("Random"))
		{
			value = random() % max;
			dirty = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		dirty |= ImGui::SliderInt(label, &value, min, max);
	};
	auto ParameterSliderFloat = [this](const char* label, float& value, float min, float max)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 0.6f, 0.6f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f).Value);

		if (ImGui::Button("Random"))
		{
			int x = random();
			value = x - max *
				static_cast<int>(x / max); // x % max;
			dirty = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		dirty |= ImGui::SliderFloat(label, &value, min, max);
	};
	auto LayerParameter = [this, ParameterSliderInt, ParameterSliderFloat,
		noiseItems, rotationItems, fractalItems, distanceItems, returnItems, domainItems](Layer& layer)
	{
		ParameterSliderInt("Seed", layer.seed, -INT_MIN / 2, INT_MAX / 2);
		ParameterSliderFloat("Frequency", layer.frequency, 0, 128.0f);
		ParameterSliderInt("Fractal Octaves", layer.fractalOctaves, 0, 8);
		ParameterSliderFloat("Fractal Lacunarity", layer.fractalLacunarity, 0.0f, 16.0f);
		ParameterSliderFloat("Fractal Gain", layer.fractalGain, 0.0f, 4.0f);
		ParameterSliderFloat("Fractal Weighted Strength", layer.fractalWeightedStrength, 0.0f, 1.0f);
		ParameterSliderFloat("Fractal Ping Pong Strength", layer.fractalPingPongStrength, 0.0f, 16.0f);
		ParameterSliderFloat("Cellular Jitter", layer.cellularJitter, 0.0f, 1.0f);
		ParameterSliderFloat("Domain Warp Amplitude", layer.domainAmplitude, 0.0f, 8.0f);

		dirty |= ImGui::Combo("Noise", &layer.noiseIndex, noiseItems.data(),
							  static_cast<int>(noiseItems.size()));
		dirty |= ImGui::Combo("Rotation", &layer.rotationIndex, rotationItems.data(),
							  static_cast<int>(rotationItems.size()));
		dirty |= ImGui::Combo("Fractal", &layer.fractalIndex, fractalItems.data(),
							  static_cast<int>(fractalItems.size()));
		dirty |= ImGui::Combo("Distance", &layer.distanceIndex, distanceItems.data(),
							  static_cast<int>(distanceItems.size()));
		dirty |= ImGui::Combo("Return", &layer.returnIndex, returnItems.data(),
							  static_cast<int>(returnItems.size()));
		dirty |= ImGui::Combo("Domain", &layer.domainIndex, domainItems.data(),
							  static_cast<int>(domainItems.size()));
	};

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Text("Voxels (%.2f mv)		Delay (%lld ms)", voxels / 1000000.0f, delay);

	std::vector<const char*> items;
	items = { "All", "Continentalness", "Erosion", "Peaks", "Temperature", "Humidity", "Wind", "Rain" };
	dirty |= ImGui::Combo("Preset", &presetTest, items.data(),
		static_cast<int>(items.size()));

	items = { "None" };
	for (const Biome& biome : biomes) items.push_back(biome.name);
	dirty |= ImGui::Combo("Palette", &paletteTest, items.data(),
		static_cast<int>(items.size()));

	dirty |= ImGui::RadioButton("2D", &dimension, 0);
	ImGui::SameLine();
	dirty |= ImGui::RadioButton("3D", &dimension, 1);

	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");
	ParameterSliderInt("Terrain X", terrainX, 1, 1024);
	ParameterSliderInt("Terrain Y", terrainY, 1, 128);
	ParameterSliderInt("Terrain Z", terrainZ, 1, 1024);

	if (ImGui::TreeNode("Biome"))
	{
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Elevation"))
	{
		if (ImGui::TreeNode("Continental"))
		{
			LayerParameter(continentalness);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Erosion"))
		{
			LayerParameter(erosion);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Peaks"))
		{
			LayerParameter(peaks);
			ImGui::TreePop();
		}
		
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Temperature"))
	{
		LayerParameter(temperature);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Humidity"))
	{
		LayerParameter(humidity);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Wind"))
	{
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Rain"))
	{
		ImGui::TreePop();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Terrain::Tick(float deltaTime)
{
	static size_t ticks = 0;

	auto SetParameters = [this](Layer& layer)
	{
		layer.noise.SetSeed(layer.seed);
		layer.noise.SetFrequency(layer.frequency);
		layer.noise.SetNoiseType(static_cast<FastNoiseLite::NoiseType>(layer.noiseIndex));
		layer.noise.SetRotationType3D(static_cast<FastNoiseLite::RotationType3D>(layer.rotationIndex));
		layer.noise.SetFractalType(static_cast<FastNoiseLite::FractalType>(layer.fractalIndex));
		layer.noise.SetFractalOctaves(layer.fractalOctaves);
		layer.noise.SetFractalLacunarity(layer.fractalLacunarity);
		layer.noise.SetFractalGain(layer.fractalGain);
		layer.noise.SetFractalWeightedStrength(layer.fractalWeightedStrength);
		layer.noise.SetFractalPingPongStrength(layer.fractalPingPongStrength);
		layer.noise.SetCellularDistanceFunction(static_cast<FastNoiseLite::CellularDistanceFunction>(layer.distanceIndex));
		layer.noise.SetCellularReturnType(static_cast<FastNoiseLite::CellularReturnType>(layer.returnIndex));
		layer.noise.SetCellularJitter(layer.cellularJitter);
		layer.noise.SetDomainWarpType(static_cast<FastNoiseLite::DomainWarpType>(layer.domainIndex));
		layer.noise.SetDomainWarpAmp(layer.domainAmplitude);
	};

	HandleInput(deltaTime);

	// 
	//  TODO: move towards a pipeline arcitecture,
	//  eg. allocate volume, 2d layer generator (take in previous layers),
	//  produce biomes from biome function, use biome to 
	// 

	// Gather noise data
	if (dirty)
	{
		SetParameters(continentalness);
		SetParameters(erosion);
		SetParameters(peaks);
		SetParameters(temperature);
		SetParameters(humidity);

		voxels = 0;
		ClearWorld();
		auto start = std::chrono::system_clock::now();
		
		for (int x = 0; x < terrainX; x++)
		{
			for (int z = 0; z < terrainZ; z++)
			{
				float fx = static_cast<float>(x),
					fz = static_cast<float>(z);

				float continentalnessNoise =
					continentalness.noise.GetNoise(fx, fz);
				float erosionNoise =
					erosion.noise.GetNoise(fx, fz);
				float peaksNoise =
					peaks.noise.GetNoise(fx, fz);

				float elevationNoise = continentalnessNoise +
					erosionNoise + peaksNoise;
				float temperatureNoise =
					temperature.noise.GetNoise(fx, fz);
				float humidityNoise =
					humidity.noise.GetNoise(fx, fz);

				const Biome& biome = BiomeFunction(elevationNoise,
					temperatureNoise, humidityNoise);

				for (int y = 0; y < terrainY; y++)
				{
					if (y < (dimension ? terrainY : 1))
					{
						size_t paletteIndex = urandom() % 8;
						std::array<uint, 8> palette = paletteTest ?
							biomes[paletteTest - 1].surface[0] : biome.surface[0];
						const std::array<float, 5> presetNoise =
						{
							continentalnessNoise, erosionNoise, peaksNoise, 
							temperatureNoise, humidityNoise
						};

						if (presetTest)
						{
							paletteIndex = static_cast<size_t>
								((presetNoise[presetTest - 1] + 1.0f) * 4.0f);
							palette = PALETTE_GRAY;
						}

						if (y < (elevationNoise + 1.0f) * 16.0f)
						{
							Plot(x, y, z, palette[paletteIndex]);
						}
						voxels++;
					}
				}
			}
		}

		using namespace std::chrono;
		auto end = system_clock::now();
		auto elapsed = duration_cast<milliseconds>(end - start);
		delay = elapsed.count();
		dirty = false;
	}

	ticks++;
}

void Terrain::Postdraw()
{
	HandleInterface();
}
