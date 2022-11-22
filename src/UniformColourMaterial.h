#pragma once

#include <IMaterial.h>

class UniformColourMaterial : public IMaterial {
	public:
		Colour colour;
		UniformColourMaterial(Colour colour);
		virtual ~UniformColourMaterial();
		virtual Colour GetColour(ModelTriangle triangle, glm::vec3 point);
};