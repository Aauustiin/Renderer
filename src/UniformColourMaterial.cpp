#include <UniformColourMaterial.h>
#include <ModelTriangle.h>

UniformColourMaterial::UniformColourMaterial(Colour colour) : colour(colour) {
	recievesShadow = true;
}

UniformColourMaterial::~UniformColourMaterial() {}

Colour UniformColourMaterial::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) {
	return colour;
}