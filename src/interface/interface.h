#pragma once

#include "src/world/layer.h"

void SetParameters(Layer& layer);
bool ParameterSliderInt(const char* label, int& value, int min, int max);
bool ParameterSliderFloat(const char* label, float& value, float min, float max);
bool ParameterCurveEditor(Layer& layer);
bool LayerParameter(Layer& layer);