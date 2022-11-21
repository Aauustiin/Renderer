#include "ModelTriangle.h"
#include <utility>

ModelTriangle::ModelTriangle() = default;

ModelTriangle::ModelTriangle(Vertex v0, Vertex v1, Vertex v2, Colour c, glm::vec3 normal) :
		vertices({{v0, v1, v2}}), colour(std::move(c)), normal(normal) {}

std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle) {
	os << "(" << triangle.vertices[0].position.x << ", " << triangle.vertices[0].position.y << ", " << triangle.vertices[0].position.z << ")\n";
	os << "(" << triangle.vertices[1].position.x << ", " << triangle.vertices[1].position.y << ", " << triangle.vertices[1].position.z << ")\n";
	os << "(" << triangle.vertices[2].position.x << ", " << triangle.vertices[2].position.y << ", " << triangle.vertices[2].position.z << ")\n";
	return os;
}
