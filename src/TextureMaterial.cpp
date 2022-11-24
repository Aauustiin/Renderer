#include <TextureMaterial.h>
#include <ModelTriangle.h>
#include <Utilities.h>

TextureMaterial::TextureMaterial(TextureMap texture) : texture(texture) {
	recievesShadow = true;
}

TextureMaterial::~TextureMaterial() {}

Colour TextureMaterial::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) {
	ModelTriangle triangle = model[triangleIndex];
	glm::vec2 texturePoint = triangleInterpolation(triangle.vertices[0].position,
		triangle.vertices[1].position,
		triangle.vertices[2].position,
		triangle.vertices[0].texturePoint,
		triangle.vertices[1].texturePoint,
		triangle.vertices[2].texturePoint,
		point);
	return texture.GetValue(texturePoint);
}