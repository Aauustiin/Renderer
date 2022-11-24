#pragma once

#include <vector>
#include <ModelTriangle.h>
#include <glm/glm.hpp>
#include <DrawingWindow.h>
#include <Objects.h>

void rayTracedRender(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> light,
	DrawingWindow& window,
	Camera cam,
	LightingMode lightingMode);