#include <RefractiveMaterial.h>
#include <ModelTriangle.h>
#include <Utilities.h>
#include <Raytracing.h>
#include <RayTriangleIntersection.h>

RefractiveMaterial::RefractiveMaterial() { recievesShadow = false; }

RefractiveMaterial::~RefractiveMaterial() {}

Colour RefractiveMaterial::GetColour(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	Camera cam,
	LightingMode lightingMode,
	int triangleIndex, glm::vec3 point) {

	glm::vec3 normal = model[triangleIndex].normal;
	glm::vec3 unitCameraToPoint = glm::normalize(point);
	// Rr = Ri - 2N(Ri . N)
	glm::vec3 reflection = unitCameraToPoint - (2.0f * normal * glm::dot(unitCameraToPoint, normal));
	RayTriangleIntersection intersection = getClosestIntersection(point, glm::normalize(reflection), model, triangleIndex);
	intersection.intersectionPoint += point;
	Colour colour = Colour(0, 0, 0);
	if ((intersection.distance < std::numeric_limits<float>::max()) && (intersection.intersectedTriangle.material->recievesShadow)) {
		float brightness = calculateBrightness(intersection, lightingMode, model, lights);
		colour = intersection.intersectedTriangle.GetColour(model, lights, cam, lightingMode,
			intersection.triangleIndex,
			intersection.intersectionPoint);
		colour.red *= brightness;
		colour.green *= brightness;
		colour.blue *= brightness;
	}
	return colour;
}

glm::vec3 findTransmissionVector(float ri, glm::vec3 normal, glm::vec3 incidence) {
	return glm::vec3(0, 0, 0);
}