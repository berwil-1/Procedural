#include "precomp.h"
#include "terrain.h"
#include "world/biome.h"
#include "math/random.h"

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
		0.1f,
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
		1.0f
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
			value = abs(random()) % max;
			dirty = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::SliderInt(label, &value, min, max);
		dirty |= ImGui::IsItemDeactivatedAfterEdit();
	};
	auto ParameterSliderFloat = [this](const char* label, float& value, float min, float max)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 0.6f, 0.6f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f).Value);

		if (ImGui::Button("Random"))
		{
			int x = random();
			value = x - max * static_cast<int>(x / max); // x % max;
			dirty = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::SliderFloat(label, &value, min, max);
		dirty |= ImGui::IsItemDeactivatedAfterEdit();
	};
	auto ParameterCurveEditor = [this](Layer& layer)
	{
		int count = 70;
		ImVec2 cursorFirst = ImGui::GetCursorPos();
		ImVec2 cursorFirstScreen = ImGui::GetCursorScreenPos();

		ImGui::PlotLines("Amplitude", layer.points.data(), 20, 0, NULL, 0.0f, 1.0f, ImVec2(0, 80));
		ImVec2 cursorSecond = ImGui::GetCursorPos();
		ImGui::SetCursorPos(cursorFirst);

		static size_t held = -1;

		if (held == -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
		{
			size_t newIndex = -1;
			float newLength = FLT_MAX;

			for(size_t index = 0; index < layer.points.size(); index++)
			{
				const float point = layer.points[index];
				ImVec2 mouse = ImGui::GetMousePos();
				mouse.x -= 10;
				mouse.y -= 5;

				// Relative coordinates of mouse
				float relativex = (mouse.x - cursorFirstScreen.x) /
					(ImGui::GetItemRectSize().x - ImGui::CalcTextSize("Amplitude").x);
				float relativey = (mouse.y - cursorFirstScreen.y) / ImGui::GetItemRectSize().y;

				ImVec2 difference = ImVec2((relativex - (index / 20.0f)) * 2.0f, (1.0f - relativey) - point);
				float length = sqrt(difference.x * difference.x + difference.y * difference.y);

				if (length < newLength)
				{
					newIndex = index;
					newLength = length;
				}
			}

			if (newLength < 0.1f)
			{
				held = newIndex;
			}
		}

		if (held != -1 && ImGui::IsItemHovered())
		{
			ImVec2 mouse = ImGui::GetMousePos();
			ImVec2 relative = ImVec2((mouse.x - cursorFirstScreen.x) / (ImGui::GetItemRectSize().x - ImGui::CalcTextSize("Amplitude").x),
				(mouse.y - cursorFirstScreen.y) / ImGui::GetItemRectSize().y);
			layer.points[held] = 1.0f - relative.y;
		}

		if (held != -1 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			held = -1;
			dirty = true;
		}

		ImGui::SetCursorPos(cursorSecond);
	};
	auto LayerParameter = [this, ParameterSliderInt, ParameterSliderFloat, ParameterCurveEditor,
		noiseItems, rotationItems, fractalItems, distanceItems, returnItems, domainItems](Layer& layer)
	{
		ParameterSliderInt("Seed", layer.seed, -INT_MIN / 2, INT_MAX / 2);
		ParameterSliderFloat("Frequency", layer.frequency, 0, 0.1f);
		ParameterSliderInt("Fractal Octaves", layer.fractalOctaves, 0, 8);
		ParameterSliderFloat("Fractal Lacunarity", layer.fractalLacunarity, 0.0f, 8.0f);
		ParameterSliderFloat("Fractal Gain", layer.fractalGain, 0.0f, 4.0f);
		ParameterSliderFloat("Fractal Weighted Strength", layer.fractalWeightedStrength, 0.0f, 1.0f);
		ParameterSliderFloat("Fractal Ping Pong Strength", layer.fractalPingPongStrength, 0.0f, 8.0f);
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

		ParameterCurveEditor(layer);
	};

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

	dirty |= ImGui::Checkbox("Waterfill", &waterfill);
	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");
	ParameterSliderInt("Terrain X", terrainX, 0, 1024);
	ParameterSliderInt("Terrain Z", terrainZ, 0, 1024);
	ParameterSliderInt("Terrain Offset X", terrainOffsetX, 0, 8192);
	ParameterSliderInt("Terrain Offset Z", terrainOffsetZ, 0, 8192);

	if (ImGui::TreeNode("Biome"))
	{
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Elevation"))
	{
		if (ImGui::TreeNode("Continental"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				LayerParameter(continentalness);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Erosion"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				LayerParameter(erosion);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Peaks"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				LayerParameter(peaks);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Climate"))
	{
		if (ImGui::TreeNode("Temperature"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				LayerParameter(temperature);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Humidity"))
		{
			if (ImGui::TreeNode("Noise"))
			{
				LayerParameter(humidity);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

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
	auto LerpPoints = [](Layer& layer, float x) -> float
	{
		std::array<float, 20>& points = layer.points;
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
	};
	auto LerpColors = [](uint16_t a, uint16_t b, float t) -> uint16_t
	{
		uint16_t ra = (a >> 8);
		uint16_t rb = (b >> 8);
		uint16_t rl = static_cast<uint16_t>(lerp(ra, rb, t));

		uint16_t ga = (a >> 4) & 15;
		uint16_t gb = (b >> 4) & 15;
		uint16_t gl = static_cast<uint16_t>(lerp(ga, gb, t));

		uint16_t ba = a & 15;
		uint16_t bb = b & 15;
		uint16_t bl = static_cast<uint16_t>(lerp(ba, bb, t));

		return (rl << 8) | (gl << 4) | bl;
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
		
#if 1
		std::array<std::array<std::pair<int, uint16_t>,
			1024>, 1024>* world = new std::array<std::array<std::pair<int, uint16_t>, 1024>, 1024>();

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

				if (waterfill && elevationNoise < 60.0f)
				{
					limit = 60.0f;
				}

				(*world)[x][z] = { limit, elevationNoise < 60.0f ? (0x006 + (static_cast<int>(0x006 * elevationNoise / 60.0f) << 4)) : colors[biome]};
			}
		}

		for (int x = 0; x < terrainX; x++)
		{
			for (int z = 0; z < terrainZ; z++)
			{
				const int limit = (*world)[x][z].first;
				const uint16_t color =
					LerpColors
					(
						LerpColors((*world)[max(x - 4, 0)][z].second, (*world)[min(x + 4, 1023)][z].second, 0.5f),
						LerpColors((*world)[x][max(z - 4, 0)].second, (*world)[x][min(z + 4, 1023)].second, 0.5f),
						0.5f
					);

				for (int y = limit; y > -1; y--)
				{
					Plot(x, y, z, color);
					voxels++;
				}
			}
		}

		delete world;
#else
		for (int x = 0; x < colors.size(); x++)
		{
			for (int y = 30; y > -1; y--)
			{
				Plot(x, y, 0, LerpColors(colors[x], 0xFFF, 1.0f - y / 30.0f));
				voxels++;
			}
		}
#endif

		using namespace std::chrono;
		auto end = system_clock::now();
		auto elapsed = duration_cast<milliseconds>(end - start);
		delay = elapsed.count();
		dirty = false;
	}

	ticks++;
}

void Tmpl8::Terrain::Predraw()
{
	// Empty
}

void Terrain::Postdraw()
{
	HandleInterface();
}

void Tmpl8::Terrain::Shutdown()
{
	FILE* f;

	f = fopen("layers.dat", "wb");
	fwrite(&continentalness, 1, sizeof(continentalness), f);
	fwrite(&erosion, 1, sizeof(erosion), f);
	fwrite(&peaks, 1, sizeof(peaks), f);
	fwrite(&temperature, 1, sizeof(temperature), f);
	fwrite(&humidity, 1, sizeof(humidity), f);
	fclose(f);

	f = fopen("camera.dat", "wb");
	fwrite(&cameraDirection, 1, sizeof(cameraDirection), f);
	fwrite(&cameraPosition, 1, sizeof(cameraPosition), f);
	fclose(f);

	// uint sprite = CreateSprite(0, 0, 0, 256, 256, 256);
	// SaveSprite(sprite, "world.vox");
}