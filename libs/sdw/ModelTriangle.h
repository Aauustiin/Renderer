#pragma once

#include <glm/glm.hpp>
#include <string>
#include <array>
#include "Colour.h"
#include <Objects.h>

struct ModelTriangle {
	std::array<Vertex, 3> vertices{};
	Colour colour{};
	glm::vec3 normal{};

	ModelTriangle();
	ModelTriangle(Vertex v0, Vertex v1, Vertex v2, Colour c, glm::vec3 normal);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
