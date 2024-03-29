#pragma once

#include <IMaterial.h>
#include <TextureMap.h>

class TextureMaterial : public IMaterial {
public:
	TextureMap texture;
	TextureMaterial(TextureMap texture);
	virtual ~TextureMaterial();
	virtual Colour GetColour(std::vector<ModelTriangle> model,
		std::vector<glm::vec3> lights,
		Camera cam,
		LightingMode lightingMode,
		int triangleIndex, glm::vec3 point);
};