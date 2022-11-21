#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

enum RenderMode {
	POINTCLOUD,
	WIREFRAME,
	RASTERISED,
	RAYTRACED
};

enum LightingMode {
	HARD,
	PROXIMITY,
	INCIDENCE,
	SPECULAR,
	AMBIENT,
	GOURAUD,
	PHONG
};

struct Camera {
	glm::vec3 position;
	glm::mat3 orientation;
	float focalLength;
};

struct RendererState {
	RenderMode renderMode;
	LightingMode lightingMode;
	bool orbiting;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texturePoint;
	float brightness;
};