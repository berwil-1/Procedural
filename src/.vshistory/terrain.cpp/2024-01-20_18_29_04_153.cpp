#include "precomp.h"
#include "terrain.h"
#include "world/biome.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Game* CreateGame() { return new Terrain(); }

void Terrain::Init()
{
	skyDomeLightScale = 6.0f;
	skyDomeImage = "assets/sky.hdr";

	// Restore camera, if possible
	FILE* f = fopen("camera.dat", "rb");
	if (f)
	{
		fread(&cameraDirection, 1, sizeof(cameraDirection), f);
		fread(&cameraPosition, 1, sizeof(cameraPosition), f);
		fclose(f);
	}

	// Load spline path
	CameraPoint p;
	FILE* fp = fopen("assets/splinepath.bin", "rb");
	while (1)
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
	auto ParameterSlider = [this](const char* label, int& value, int min, int max)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 0.6f, 0.6f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f).Value);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f).Value);

		if (ImGui::Button("Random"))
		{
			value = (rand() ^ (rand() << 15) ^ (rand() << 30)) % max;
			dirty = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		dirty |= ImGui::SliderInt(label, &value, min, max);
	};

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const char* items[] = { "Biome", "Evaluation", "Temperature", "Humidity", "Wind", "Rain" };
	ImGui::Combo("Preset", &preset, items, IM_ARRAYSIZE(items));

	ImGui::RadioButton("2D", &dimension, 0);
	ImGui::SameLine();
	ImGui::RadioButton("3D", &dimension, 1);

	ImGui::NewLine();

	ImGui::SeparatorText("Terrain");

	ParameterSlider("Seed", seed, -INT_MIN / 2, INT_MAX / 2);

	if (ImGui::TreeNode("Biome"))
	{
		ParameterSlider("Parameter", a, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Evaluation"))
	{
		ParameterSlider("Parameter", b, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Temperature"))
	{
		ParameterSlider("Parameter", c, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Humidity"))
	{
		ParameterSlider("Parameter", d, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Wind"))
	{
		ParameterSlider("Parameter", e, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Rain"))
	{
		ParameterSlider("Parameter", f, -INT_MIN / 2, INT_MAX / 2);
		ImGui::TreePop();
	}

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
		noise.SetSeed(seed);
		noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

		for (int z = 0; z < 512; z++)
		{
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 512; x++)
				{
					if (y < (noise.GetNoise((float)x, (float)z) + 1.0f) * 8.0f)
					{
						if ((y + 1) < (noise.GetNoise((float)x, (float)z) + 1.0f) * 8.0f)
							Plot(x, y, z, 0x531);
						else
							Plot(x, y, z, 0x570);
					}
					else
						Plot(x, y, z, 0);
				}
			}
		}

		dirty = false;
	}

	ticks++;
}

void Terrain::Postdraw()
{
	HandleInterface();
}
