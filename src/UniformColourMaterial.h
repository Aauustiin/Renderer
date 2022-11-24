#pragma once

#include <IMaterial.h>

class UniformColourMaterial : public IMaterial {
	public:
		Colour colour;
		UniformColourMaterial(Colour colour);
		virtual ~UniformColourMaterial();
		virtual Colour GetColour(std::vector<ModelTriangle> model,
			std::vector<glm::vec3> lights,
			Camera cam,
			LightingMode lightingMode,
			int triangleIndex, glm::vec3 point);
};