#pragma once

#include <Colour.h>
#include <glm/glm.hpp>
#include <vector>
#include <Objects.h>

struct ModelTriangle;

class IMaterial {
	public:
		bool recievesShadow;
		IMaterial();
		virtual ~IMaterial() = 0;
		virtual Colour GetColour(std::vector<ModelTriangle> model,
			std::vector<glm::vec3> lights,
			Camera cam,
			LightingMode lightingMode,
			int triangleIndex, glm::vec3 point) = 0;
};