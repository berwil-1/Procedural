#include "interface.h"
#include "precomp.h"

#include "src/math/random.h"
#include "src/world/layer.h"

#include "imgui.h"

#include <array>
#include <cmath>

void SetParameters(Layer& layer)
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
}

bool ParameterSliderInt(const char* label, int& value, int min, int max)
{
	bool dirty = false;

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
	return dirty;
}

bool ParameterSliderFloat(const char* label, float& value, float min, float max)
{
	bool dirty = false;

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
	ImGui::SliderFloat(label, &value, min, max, "%.5f");
	dirty |= ImGui::IsItemDeactivatedAfterEdit();
	return dirty;
}

bool ParameterCurveEditor(Layer& layer)
{
	bool dirty = false;
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

		for (size_t index = 0; index < layer.points.size(); index++)
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

		if (newLength < 0.2f)
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
	return dirty;
}

bool LayerParameter(Layer& layer)
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

	bool dirty = false;

	dirty |= ParameterSliderInt("Seed", layer.seed, -INT_MIN / 2, INT_MAX / 2);
	dirty |= ParameterSliderFloat("Frequency", layer.frequency, 0, 0.1f);
	dirty |= ParameterSliderInt("Fractal Octaves", layer.fractalOctaves, 0, 8);
	dirty |= ParameterSliderFloat("Fractal Lacunarity", layer.fractalLacunarity, 0.0f, 8.0f);
	dirty |= ParameterSliderFloat("Fractal Gain", layer.fractalGain, 0.0f, 4.0f);
	dirty |= ParameterSliderFloat("Fractal Weighted Strength", layer.fractalWeightedStrength, 0.0f, 1.0f);
	dirty |= ParameterSliderFloat("Fractal Ping Pong Strength", layer.fractalPingPongStrength, 0.0f, 8.0f);
	dirty |= ParameterSliderFloat("Cellular Jitter", layer.cellularJitter, 0.0f, 1.0f);
	dirty |= ParameterSliderFloat("Domain Warp Amplitude", layer.domainAmplitude, 0.0f, 8.0f);

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
	
	dirty |= ParameterCurveEditor(layer);

	return dirty;
};
