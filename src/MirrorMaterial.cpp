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
	// Rr = Ri - 2N(Ri . N)
	glm::vec3 reflection = glm::normalize(unitCameraToPoint - (2.0f * normal * glm::dot(unitCameraToPoint, normal)));
	RayTriangleIntersection intersection = getClosestIntersection(point, glm::normalize(reflection), model, triangleIndex);
	intersection.intersectionPoint += point;
	Colour colour = Colour(0, 0, 0);
	if ((intersection.distance < std::numeric_limits<float>::max()) && (intersection.intersectedTriangle.material->recievesShadow)) {
		float brightness = calculateBrightness(intersection, lightingMode, model, lights);
		colour = intersection.intersectedTriangle.GetColour(model, lights, cam, lightingMode,
			intersection.triangleIndex,
			intersection.intersectionPoint);
		colour.red *= 0.9;
		colour.green *= 0.9;
		colour.blue *= 0.9;
		colour.blue += 0.1 * 255;
		colour.red *= brightness;
		colour.green *= brightness;
		colour.blue *= brightness;
	}
	return colour;
}