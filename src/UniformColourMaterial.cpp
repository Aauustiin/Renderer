#include <UniformColourMaterial.h>
#include <ModelTriangle.h>

UniformColourMaterial::UniformColourMaterial(Colour colour) : colour(colour) {}

UniformColourMaterial::~UniformColourMaterial() {}

Colour UniformColourMaterial::GetColour(ModelTriangle triangle, glm::vec3 point) {
	return colour;
}