#include <ModelTriangle.h>

ModelTriangle::ModelTriangle() = default;

ModelTriangle::ModelTriangle(Vertex v0, Vertex v1, Vertex v2, IMaterial * mat, glm::vec3 normal) :
		vertices({{v0, v1, v2}}), material(mat), normal(normal) {}

Colour ModelTriangle::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) {
	return material->GetColour(model, lights, cam, lightingMode, triangleIndex, point);
}

std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle) {
	os << "(" << triangle.vertices[0].position.x << ", " << triangle.vertices[0].position.y << ", " << triangle.vertices[0].position.z << ")\n";
	os << "(" << triangle.vertices[1].position.x << ", " << triangle.vertices[1].position.y << ", " << triangle.vertices[1].position.z << ")\n";
	os << "(" << triangle.vertices[2].position.x << ", " << triangle.vertices[2].position.y << ", " << triangle.vertices[2].position.z << ")\n";
	return os;
}
