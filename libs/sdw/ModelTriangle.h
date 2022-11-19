#pragma once

#include <glm/glm.hpp>
#include <string>
#include <array>
#include "Colour.h"
#include "TexturePoint.h"

struct ModelTriangle {
	std::array<glm::vec3, 3> vertices{};
	std::array<glm::vec3, 3> vertexNormals{};
	std::array<TexturePoint, 3> texturePoints{};
	Colour colour{};
	glm::vec3 normal{};

	ModelTriangle();
	ModelTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Colour trigColour);
	ModelTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, Colour trigColour,
		const glm::vec3& vn0, const glm::vec3& vn1, const glm::vec3& vn2);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
