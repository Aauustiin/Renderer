#include <TextureMaterial.h>
#include <ModelTriangle.h>
#include <Utilities.h>

TextureMaterial::TextureMaterial(TextureMap texture) : texture(texture) {}

TextureMaterial::~TextureMaterial() {}

Colour TextureMaterial::GetColour(ModelTriangle triangle, glm::vec3 point) {
	glm::vec2 texturePoint = triangleInterpolation(triangle.vertices[0].position,
		triangle.vertices[1].position,
		triangle.vertices[2].position,
		triangle.vertices[0].texturePoint,
		triangle.vertices[1].texturePoint,
		triangle.vertices[2].texturePoint,
		point);
	return texture.GetValue(texturePoint);
}