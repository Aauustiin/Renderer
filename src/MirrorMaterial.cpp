#include <MirrorMaterial.h>
#include <ModelTriangle.h>
#include <Utilities.h>
#include <Raytracing.h>
#include <RayTriangleIntersection.h>

MirrorMaterial::MirrorMaterial() { recievesShadow = false; }

MirrorMaterial::~MirrorMaterial() {}

Colour MirrorMaterial::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) {

	glm::vec3 normal = model[triangleIndex].normal;
	glm::vec3 unitCameraToPoint = glm::normalize(point);
	glm::vec3 reflection = unitCameraToPoint - (2.0f * normal * glm::dot(unitCameraToPoint, normal));
	RayTriangleIntersection intersection = getClosestIntersection(point, glm::normalize(reflection), model, triangleIndex);
	Colour colour = Colour(0, 0, 0);
	if (intersection.distance < std::numeric_limits<float>::max()) {
		float brightness = calculateBrightness(intersection, lightingMode, model, lights);
		colour = intersection.intersectedTriangle.GetColour(model, lights, cam, lightingMode, triangleIndex, point);
		colour.red *= brightness;
		colour.green *= brightness;
		colour.blue *= brightness;
	}
	return colour;
}