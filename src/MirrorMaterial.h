#pragma once

#include <IMaterial.h>

class MirrorMaterial : public IMaterial {
public:
	MirrorMaterial();
	virtual ~MirrorMaterial();
	virtual Colour GetColour(std::vector<ModelTriangle> model,
		std::vector<glm::vec3> lights,
		Camera cam,
		LightingMode lightingMode,
		int triangleIndex, glm::vec3 point);
};