#pragma once

#include <Colour.h>
#include <glm/glm.hpp>

struct ModelTriangle;

class IMaterial {
	public:
		IMaterial();
		virtual ~IMaterial() = 0;
		virtual Colour GetColour(ModelTriangle triangle, glm::vec3 point) = 0;
};