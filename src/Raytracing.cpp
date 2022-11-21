#include <RayTriangleIntersection.h>
#include <Raytracing.h>

#define PI 3.14159265358979323846264338327950288

RayTriangleIntersection getIntersection(glm::vec3 startPosition, glm::vec3 direction, ModelTriangle target) {
	RayTriangleIntersection result = RayTriangleIntersection(glm::vec3(0, 0, 0),
		std::numeric_limits<float>::max(),
		ModelTriangle(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), Colour(0, 0, 0)),
		0);

	glm::vec3 e0 = target.vertices[1] - target.vertices[0];
	glm::vec3 e1 = target.vertices[2] - target.vertices[0];
	glm::vec3 SPVector = startPosition - target.vertices[0];
	glm::mat3 DEMatrix(-direction, e0, e1);
	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

	bool boundsCheck = ((possibleSolution.y >= 0.0) && (possibleSolution.y <= 1.0)) &&
		((possibleSolution.z >= 0.0) && (possibleSolution.z <= 1.0)) &&
		((possibleSolution.y + possibleSolution.z) <= 1.0);

	if ((possibleSolution.x > 0) && boundsCheck) {
		result = RayTriangleIntersection(direction * possibleSolution.x,
			possibleSolution.x, target,
			0);
	}

	return result;
}

RayTriangleIntersection getClosestIntersection(glm::vec3 startPosition,
	glm::vec3 direction,
	std::vector<ModelTriangle> targets,
	int indexBlacklist = std::numeric_limits<int>::max()) {

	RayTriangleIntersection result = RayTriangleIntersection(glm::vec3(0, 0, 0),
		std::numeric_limits<float>::max(),
		ModelTriangle(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), Colour(0, 0, 0)),
		0);

	for (int i = 0; i < targets.size(); i++) {
		if (i != indexBlacklist) {
			RayTriangleIntersection possibleResult = getIntersection(startPosition, direction, targets[i]);
			possibleResult.triangleIndex = i;
			if (possibleResult.distance < result.distance) {
				result = possibleResult;
			}
		}
	}

	return result;
}

float hardShadowLighting(RayTriangleIntersection intersection,
	std::vector<ModelTriangle> model,
	glm::vec3 light) {
	Colour colour;

	glm::vec3 pointToLight = light - intersection.intersectionPoint;
	RayTriangleIntersection lightIntersection = getClosestIntersection(intersection.intersectionPoint,
		glm::normalize(pointToLight),
		model,
		intersection.triangleIndex);

	float intensity = lightIntersection.distance < glm::length(pointToLight) ? 0 : 1;
	return intensity;
}

float proximityLighting(RayTriangleIntersection intersection, glm::vec3 light, float strength = 12.5) {
	float distance = glm::length(intersection.intersectionPoint - light);
	double intensity = std::min(strength / (4 * PI * distance * distance), 1.0);
	return intensity;
}

float incidenceLighting(RayTriangleIntersection intersection, glm::vec3 light, glm::vec3 normal = { 0, 0, 0 }) {
	if (normal == glm::vec3(0, 0, 0)) normal = intersection.intersectedTriangle.normal;
	glm::vec3 pointToLight = glm::normalize(light - intersection.intersectionPoint);
	float similarity = std::max(glm::dot(pointToLight, normal), 0.0f);
	return similarity;
}

float specularLighting(RayTriangleIntersection intersection, glm::vec3 light, int specularExponent = 128,
	glm::vec3 normal = { 0, 0, 0 }) {

	if (normal == glm::vec3(0, 0, 0)) normal = intersection.intersectedTriangle.normal;
	glm::vec3 unitLightToPoint = -glm::normalize(light - intersection.intersectionPoint);
	glm::vec3 reflection = unitLightToPoint - (2.0f * normal * glm::dot(unitLightToPoint, normal));

	float reflectionSimilarity = -glm::dot(glm::normalize(reflection), glm::normalize(intersection.intersectionPoint));
	reflectionSimilarity = reflectionSimilarity < 0 ? 0 : reflectionSimilarity;
	reflectionSimilarity = std::pow(reflectionSimilarity, specularExponent);
	return reflectionSimilarity;
}

float ambientLighting(float currentIntensity, float addition = 0.2) {
	return std::min(currentIntensity + addition, 1.0f);
}

float triangleArea(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
	glm::vec3 AB = v1 - v0;
	glm::vec3 AC = v2 - v0;
	float area = glm::length(glm::cross(AB, AC)) / 2;
	return area;
}

float gouraurdLighting(RayTriangleIntersection intersection, float i0, float i1, float i2) {
	float entireArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectedTriangle.vertices[2]);
	float v0OppArea = triangleArea(intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectedTriangle.vertices[2]);
	float v1OppArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[2]);
	float v2OppArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectionPoint);
	v0OppArea /= entireArea;
	v1OppArea /= entireArea;
	v2OppArea /= entireArea;
	float intensity = (i0 * v0OppArea) + (i1 * v1OppArea) + (i2 * v2OppArea);
	return intensity;
}

glm::vec3 phongLighting(RayTriangleIntersection intersection) {
	float entireArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectedTriangle.vertices[2]);
	float v0OppArea = triangleArea(intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectedTriangle.vertices[2]);
	float v1OppArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[2]);
	float v2OppArea = triangleArea(intersection.intersectedTriangle.vertices[0],
		intersection.intersectedTriangle.vertices[1],
		intersection.intersectionPoint);
	v0OppArea /= entireArea;
	v1OppArea /= entireArea;
	v2OppArea /= entireArea;
	glm::vec3 normal = (intersection.intersectedTriangle.vertexNormals[0] * v0OppArea) +
		(intersection.intersectedTriangle.vertexNormals[1] * v1OppArea) +
		(intersection.intersectedTriangle.vertexNormals[2] * v2OppArea);
	return normal;
}

void rayTracedRender(std::vector<ModelTriangle> model,
	glm::vec3 light,
	DrawingWindow& window,
	Camera cam,
	LightingMode lightingMode) {

	light = cam.orientation * (light - cam.position);

	for (int i = 0; i < model.size(); i++) {
		for (int j = 0; j < 3; j++) {
			model[i].vertices[j] = cam.orientation * (model[i].vertices[j] - cam.position);
		}
	}

	for (int i = 0; i < window.width; i++) {
		for (int j = 0; j < window.height; j++) {

			glm::vec3 direction = { (i - window.width / 2) / window.scale, (window.height / 2 - j) / window.scale, -cam.focalLength };
			direction = glm::normalize(direction);
			RayTriangleIntersection intersection = getClosestIntersection(glm::vec3(0, 0, 0), direction, model);

			float intensity = 1;
			if (intersection.distance == std::numeric_limits<float>::max())
				intensity = 0;
			else {
				switch (lightingMode) {
				case HARD:
					intensity = hardShadowLighting(intersection, model, light);
					break;
				case PROXIMITY:
					intensity = proximityLighting(intersection, light);
					break;
				case INCIDENCE:
					intensity = proximityLighting(intersection, light);
					intensity *= incidenceLighting(intersection, light);
					break;
				case SPECULAR:
					intensity = proximityLighting(intersection, light);
					intensity *= incidenceLighting(intersection, light);
					intensity += specularLighting(intersection, light);
					intensity = glm::min(intensity, 1.0f);
					break;
				case AMBIENT:
					intensity = proximityLighting(intersection, light);
					intensity *= incidenceLighting(intersection, light);
					intensity += specularLighting(intersection, light);
					intensity = glm::min(intensity, 1.0f);
					intensity *= hardShadowLighting(intersection, model, light);
					intensity = ambientLighting(intensity);
					break;
				case GOURAUD:
				{
					intensity = proximityLighting(intersection, light);
					glm::vec3 v0 = intersection.intersectedTriangle.vertices[0];
					glm::vec3 v1 = intersection.intersectedTriangle.vertices[1];
					glm::vec3 v2 = intersection.intersectedTriangle.vertices[2];
					float i0 = incidenceLighting(RayTriangleIntersection(v0,
						intersection.distance,
						intersection.intersectedTriangle,
						intersection.triangleIndex), light,
						intersection.intersectedTriangle.vertexNormals[0]);
					float i1 = incidenceLighting(RayTriangleIntersection(v1,
						intersection.distance,
						intersection.intersectedTriangle,
						intersection.triangleIndex), light,
						intersection.intersectedTriangle.vertexNormals[1]);
					float i2 = incidenceLighting(RayTriangleIntersection(v2,
						intersection.distance,
						intersection.intersectedTriangle,
						intersection.triangleIndex), light,
						intersection.intersectedTriangle.vertexNormals[2]);
					intensity *= gouraurdLighting(intersection, i0, i1, i2);
					intensity += specularLighting(intersection, light, 256);
					intensity = glm::min(intensity, 1.0f);
					intensity *= hardShadowLighting(intersection, model, light);
					intensity = ambientLighting(intensity);
					break;
				}
				case PHONG:
				{
					glm::vec3 normal = phongLighting(intersection);
					intensity = proximityLighting(intersection, light);
					intensity *= incidenceLighting(intersection, light, normal);
					intensity += specularLighting(intersection, light, 256, normal);
					intensity = glm::min(intensity, 1.0f);
					intensity *= hardShadowLighting(intersection, model, light);
					intensity = ambientLighting(intensity);
					break;
				}
				}
			}
			Colour colour = Colour(intersection.intersectedTriangle.colour.red * intensity,
				intersection.intersectedTriangle.colour.green * intensity,
				intersection.intersectedTriangle.colour.blue * intensity);
			window.setPixelColour(i, j, colour.getPackedColour());
		}
	}
}