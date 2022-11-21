#ifndef OBJECTS_H
#define OBJECTS_H

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
	// First number is the index of a face that has this vertex in it.
	// Second number is the index of this vertex in the array of vertices for that triangle.
	std::vector<std::array<int, 2>> faces;
};

#endif // !OBJECTS_H