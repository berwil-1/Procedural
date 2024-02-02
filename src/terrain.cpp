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

	parameters.dirty |= ImGui::RadioButton("2D", &parameters.dimension, 0);
	ImGui::SameLine();
	parameters.dirty |= ImGui::RadioButton("3D", &parameters.dimension, 1);

	parameters.dirty |= ImGui::Checkbox("Color blend", &parameters.blend);
	parameters.dirty |= ImGui::Checkbox("Water fill", &parameters.waterFill);
	parameters.dirty |= ImGui::Checkbox("Water erosion", &parameters.waterErosion);
	parameters.dirty |= ImGui::Checkbox("Cave inverted", &parameters.caveInverted);
	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");
	parameters.dirty |= ParameterSliderInt("Terrain X", parameters.terrainX, 0, 1024);
	parameters.dirty |= ParameterSliderInt("Terrain Z", parameters.terrainZ, 0, 1024);
	parameters.dirty |= ParameterSliderInt("Terrain Offset X", parameters.terrainOffsetX, -8192, 8192);
	parameters.dirty |= ParameterSliderInt("Terrain Offset Z", parameters.terrainOffsetZ, -8192, 8192);

	std::vector<const char*> items =
	{
		"All", "Continentalness", "Erosion", "Peaks", "Humidity",
		"Density Continental", "Density", "Density Peaks"
	};

	parameters.dirty |= ImGui::Combo("Layer", &parameters.layerIndex, items.data(),
		static_cast<int>(items.size()));

	/*std::vector<const char*> items;
	items = { "All", "Continentalness", "Erosion", "Peaks", "Temperature", "Humidity" };
	parameters.dirty |= ImGui::Combo("Preset", &parameters.presetTest, items.data(),
		static_cast<int>(items.size()));*/

		/*items = {"None"};
		for (const Biome& biome : biomes) items.push_back(biome.name);
		parameters.dirty |= ImGui::Combo("Palette", &paletteTest, items.data(),
			static_cast<int>(items.size()));*/

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
	int section = parameters.terrainX / THREAD_LIMIT;
	int start = thread * section;
	int end = start + section;

	for (int x = start; x < end; x++)
	{
		for (int z = 0; z < parameters.terrainZ; z++)
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

			if (!parameters.layerIndex)
			{
				color = 
			}

			/*if (waterErosion && level < 60)
			{
				int a = static_cast<int>(lerp((*world)[max(x - 4, 0)][z].first, (*world)[min(x + 4, 1023)][z].first, 0.5f));
				int b = static_cast<int>(lerp((*world)[x][max(z - 4, 0)].first, (*world)[x][min(z + 4, 1023)].first, 0.5f));
				level = static_cast<int>(lerp(a, b, 0.5f));
			}*/

			/*const uint16_t color =
			colorBlend ?
			LerpColors
			(
				LerpColors((*world)[max(x - 4, 0)][z].second, (*world)[min(x + 4, 1023)][z].second, 0.5f),
				LerpColors((*world)[x][max(z - 4, 0)].second, (*world)[x][min(z + 4, 1023)].second, 0.5f),
				0.5f
			) : (*world)[x][z].second;*/

			const float fx = static_cast<float>(x + 0),
				fz = static_cast<float>(z + 0);

			float contdensityNoise =
				LerpPoints(contdensity, (contdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * contdensity.noise.GetNoise(fx, fz);
			float peakdensityNoise =
				LerpPoints(peakdensity, (peakdensity.noise.GetNoise(fx, fz) + 1.0f) / 2.0f) * peakdensity.noise.GetNoise(fx, fz);

			for (int y = level; y > -1; y--)
			{
				float fy = static_cast<float>(y);

				bool bounds = y < 40 + peakdensityNoise * 4.0f &&
					y > 36 + peakdensityNoise * 4.0f;
				bool noodle = abs(contdensityNoise * 10.0f +
					density.noise.GetNoise(fx, fy, fz) * 5.0f +
					peakdensity.noise.GetNoise(fx, fy, fz)) < 0.5f;

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

		// TODO: rework presets into pre defined sets of parameters
		// TODO: move preset logic into layer and make it work again
		// TODO: camera lerp
		// TODO: add dirt and stone underground
		// TODO: document and write presentation

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
				uint8_t level = static_cast<uint8_t>(parameters.dimension ? elevationNoise : 1);

				// Fill air with water
				if (parameters.waterFill &&
					parameters.dimension &&
					elevationNoise < 60.0f)
				{
					level = 60;
				}

				// Not entirely accurate,
				// but much easier
				voxels += level;

				(*world)[x][z] =
				{
					level,
					biome
				};
			}
		}

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