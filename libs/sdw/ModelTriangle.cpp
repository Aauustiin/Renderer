#include <ModelTriangle.h>

ModelTriangle::ModelTriangle() = default;

ModelTriangle::ModelTriangle(Vertex v0, Vertex v1, Vertex v2, IMaterial * mat, glm::vec3 normal) :
		vertices({{v0, v1, v2}}), material(mat), normal(normal) {}

Colour ModelTriangle::GetColour(glm::vec3 point) {
	return material->GetColour((*this), point);
}

std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle) {
	os << "(" << triangle.vertices[0].position.x << ", " << triangle.vertices[0].position.y << ", " << triangle.vertices[0].position.z << ")\n";
	os << "(" << triangle.vertices[1].position.x << ", " << triangle.vertices[1].position.y << ", " << triangle.vertices[1].position.z << ")\n";
	os << "(" << triangle.vertices[2].position.x << ", " << triangle.vertices[2].position.y << ", " << triangle.vertices[2].position.z << ")\n";
	return os;
}
