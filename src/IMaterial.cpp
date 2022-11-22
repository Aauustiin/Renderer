#include <IMaterial.h>
#include <ModelTriangle.h>

IMaterial::IMaterial() {}
IMaterial::~IMaterial() {}
Colour IMaterial::GetColour(ModelTriangle triangle, glm::vec3 point) { return Colour(0,0,0); }