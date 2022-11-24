#include <IMaterial.h>
#include <ModelTriangle.h>

IMaterial::IMaterial() {}
IMaterial::~IMaterial() {}
Colour IMaterial::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) { return Colour(0,0,0); }