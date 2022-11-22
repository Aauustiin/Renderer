#pragma once

#include <IMaterial.h>
#include <TextureMap.h>

class TextureMaterial : public IMaterial {
public:
	TextureMap texture;
	TextureMaterial(TextureMap texture);
	virtual ~TextureMaterial();
	virtual Colour GetColour(ModelTriangle triangle, glm::vec3 point);
};