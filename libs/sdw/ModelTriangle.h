#pragma once

#include <glm/glm.hpp>
#include <string>
#include <array>
#include <Objects.h>
#include <IMaterial.h>

struct ModelTriangle {
	std::array<Vertex, 3> vertices{};
	IMaterial* material{};
	glm::vec3 normal{};

	ModelTriangle();
	ModelTriangle(Vertex v0, Vertex v1, Vertex v2, IMaterial* mat, glm::vec3 normal);
	Colour GetColour(std::vector<ModelTriangle> model,
		std::vector<glm::vec3> lights,
		Camera cam,
		LightingMode lightingMode,
		int triangleIndex, glm::vec3 point);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
