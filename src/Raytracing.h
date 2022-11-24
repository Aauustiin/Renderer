#pragma once

#include <vector>
#include <ModelTriangle.h>
#include <glm/glm.hpp>
#include <DrawingWindow.h>
#include <Objects.h>
#include <RayTriangleIntersection.h>

void rayTracedRender(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> light,
	DrawingWindow& window,
	Camera cam,
	LightingMode lightingMode);

RayTriangleIntersection getClosestIntersection(glm::vec3 startPosition,
	glm::vec3 direction,
	std::vector<ModelTriangle> targets,
	int indexBlacklist = std::numeric_limits<int>::max());

float calculateBrightness(RayTriangleIntersection intersection,
	LightingMode lightingMode,
	std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights);