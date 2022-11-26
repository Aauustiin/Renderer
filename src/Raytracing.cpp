#include <RayTriangleIntersection.h>
#include <Raytracing.h>
#include <Utilities.h>
#include <UniformColourMaterial.h>

#define PI 3.14159265358979323846264338327950288

RayTriangleIntersection getIntersection(glm::vec3 startPosition, glm::vec3 direction, ModelTriangle target) {
	RayTriangleIntersection result = RayTriangleIntersection(glm::vec3(0, 0, 0),
		std::numeric_limits<float>::max(),
		ModelTriangle(Vertex(), Vertex(), Vertex(),
			&UniformColourMaterial(Colour(0, 0, 0)),
			glm::vec3(0, 0, 0)),
		0);

	glm::vec3 e0 = target.vertices[1].position - target.vertices[0].position;
	glm::vec3 e1 = target.vertices[2].position - target.vertices[0].position;
	glm::vec3 SPVector = startPosition - target.vertices[0].position;
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
	int indexBlacklist) {

	RayTriangleIntersection result = RayTriangleIntersection(glm::vec3(0, 0, 0),
		std::numeric_limits<float>::max(),
		ModelTriangle(Vertex(), Vertex(), Vertex(), 
			&UniformColourMaterial(Colour(0, 0, 0)), 
			glm::vec3(0, 0, 0)),
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

// Returns the brightness of a point given the brightness of the 3 vertices of a triangle.
float interpolateBrightness(RayTriangleIntersection intersection) {
	float entireArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v0OppArea = triangleArea(intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v1OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[2].position);
	float v2OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectionPoint);
	v0OppArea /= entireArea;
	v1OppArea /= entireArea;
	v2OppArea /= entireArea;
	float brightness = (intersection.intersectedTriangle.vertices[0].brightness * v0OppArea) + 
		(intersection.intersectedTriangle.vertices[1].brightness * v1OppArea) + 
		(intersection.intersectedTriangle.vertices[2].brightness * v2OppArea);
	return brightness;
}

float hardShadowLighting(RayTriangleIntersection intersection,
	std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights) {

	float brightness = 0;
	float brightnessPerLight = 1.0f / lights.size();
	for (int i = 0; i < lights.size(); i++) {
		glm::vec3 pointToLight = lights[i] - intersection.intersectionPoint;
		RayTriangleIntersection lightIntersection = getClosestIntersection(intersection.intersectionPoint,
			glm::normalize(pointToLight),
			model,
			intersection.triangleIndex);

		brightness += lightIntersection.distance < glm::length(pointToLight) ? 0 : brightnessPerLight;
	}
	return brightness;
}

float vertexHardShadowLighting(RayTriangleIntersection intersection,
	std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights) {

	glm::vec3 v0 = intersection.intersectedTriangle.vertices[0].position;
	glm::vec3 v1 = intersection.intersectedTriangle.vertices[1].position;
	glm::vec3 v2 = intersection.intersectedTriangle.vertices[2].position;

	RayTriangleIntersection v0Intersection = getClosestIntersection(v0,
		glm::vec3(0,0,0),
		model,
		intersection.triangleIndex);
	RayTriangleIntersection v1Intersection = getClosestIntersection(v1,
		glm::vec3(0,0,0),
		model,
		intersection.triangleIndex);
	RayTriangleIntersection v2Intersection = getClosestIntersection(v2,
		glm::vec3(0,0,0),
		model,
		intersection.triangleIndex);

	intersection.intersectedTriangle.vertices[0].brightness = hardShadowLighting(v0Intersection, model, lights);
	intersection.intersectedTriangle.vertices[1].brightness = hardShadowLighting(v1Intersection, model, lights);
	intersection.intersectedTriangle.vertices[2].brightness = hardShadowLighting(v2Intersection, model, lights);

	return interpolateBrightness(intersection);
}

float proximityLighting(RayTriangleIntersection intersection, glm::vec3 light, float strength = 12.5) {
	float distance = glm::length(intersection.intersectionPoint - light);
	float brightness = strength / (4 * PI * distance * distance);
	brightness = std::min(brightness, 1.0f);
	return brightness;
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

float gouraudLighting(RayTriangleIntersection intersection, float i0, float i1, float i2) {
	float entireArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v0OppArea = triangleArea(intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v1OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[2].position);
	float v2OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectionPoint);
	v0OppArea /= entireArea;
	v1OppArea /= entireArea;
	v2OppArea /= entireArea;
	float intensity = (i0 * v0OppArea) + (i1 * v1OppArea) + (i2 * v2OppArea);
	return intensity;
}

glm::vec3 phongLighting(RayTriangleIntersection intersection) {
	float entireArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v0OppArea = triangleArea(intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectedTriangle.vertices[2].position);
	float v1OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectionPoint,
		intersection.intersectedTriangle.vertices[2].position);
	float v2OppArea = triangleArea(intersection.intersectedTriangle.vertices[0].position,
		intersection.intersectedTriangle.vertices[1].position,
		intersection.intersectionPoint);
	v0OppArea /= entireArea;
	v1OppArea /= entireArea;
	v2OppArea /= entireArea;
	glm::vec3 normal = (intersection.intersectedTriangle.vertices[0].normal * v0OppArea) +
		(intersection.intersectedTriangle.vertices[1].normal * v1OppArea) +
		(intersection.intersectedTriangle.vertices[2].normal * v2OppArea);
	return normal;
}

float calculateBrightness(RayTriangleIntersection intersection,
	LightingMode lightingMode,
	std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights) {
	float intensity = 1;
	glm::vec3 light = lights[0];
	if (intersection.distance == std::numeric_limits<float>::max())
		intensity = 0;
	else {
		switch (lightingMode) {
		case HARD:
			intensity = hardShadowLighting(intersection, model, lights);
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
		{
			intensity = 0;
			// Pick half the points at random and use those!
			std::vector<int> samples = {};
			int numSamples = std::round((lights.size() + 1) / 2);
			for (int i = 0; i < std::round(numSamples); i++) {
				int randomIndex = rand() % lights.size();
				samples.push_back(randomIndex);
			}
			for (int s : samples) {
				float a;
				a = proximityLighting(intersection, lights[s]);
				a *= incidenceLighting(intersection, lights[s]);
				a += specularLighting(intersection, lights[s]);
				a = glm::min(a, 1.0f);
				a *= hardShadowLighting(intersection, model, { lights[s] });
				a = ambientLighting(a);
				intensity += a;
			}
			intensity /= numSamples;
			break;
		}
		case GOURAUD:
		{
			//intensity = interpolateBrightness(intersection);
			//intensity *= vertexHardShadowLighting(intersection, model, light);
			//intensity = ambientLighting(intensity);
			break;
		}
		case PHONG:
		{
			glm::vec3 normal = phongLighting(intersection);
			intensity = proximityLighting(intersection, light);
			intensity *= incidenceLighting(intersection, light, normal);
			intensity += specularLighting(intersection, light, 256, normal);
			intensity = glm::min(intensity, 1.0f);
			intensity *= vertexHardShadowLighting(intersection, model, lights);
			intensity = ambientLighting(intensity);
			break;
		}
		}
	}
	return intensity;
}

void rayTracedRender(std::vector<ModelTriangle> model,
	std::vector<glm::vec3> lights,
	DrawingWindow& window,
	Camera cam,
	LightingMode lightingMode) {

	for (int i = 0; i < lights.size(); i++) {
		lights[i] = cam.orientation * (lights[i] - cam.position);
	}
	glm::vec3 light = lights[0];

	for (int i = 0; i < model.size(); i++) {
		for (int j = 0; j < 3; j++) {
			model[i].vertices[j].position = cam.orientation * (model[i].vertices[j].position - cam.position);
			model[i].vertices[j].normal = cam.orientation * model[i].vertices[j].normal;
		}
		model[i].normal = cam.orientation * model[i].normal;
	}

	if (lightingMode == GOURAUD) {
		for (int i = 0; i < model.size(); i++) {
			for (int j = 0; j < 3; j++) {
				glm::vec3 vertexPosition = model[i].vertices[j].position;
				RayTriangleIntersection intersection = RayTriangleIntersection(vertexPosition,
					glm::length(vertexPosition),
					model[i],
					i);
				float brightness = proximityLighting(intersection, light, 5.0f);
				brightness = incidenceLighting(intersection, light, model[i].vertices[j].normal);
				//brightness += specularLighting(intersection, light, 256, model[i].vertices[j].normal);
				//brightness = glm::min(brightness, 1.0f);
				model[i].vertices[j].brightness = brightness;
			}
		}
	}

	for (int i = 0; i < window.width; i++) {
		for (int j = 0; j < window.height; j++) {

			glm::vec3 direction = { (i - window.width / 2) / window.scale, (window.height / 2 - j) / window.scale, -cam.focalLength };
			direction = glm::normalize(direction);
			RayTriangleIntersection intersection = getClosestIntersection(glm::vec3(0, 0, 0), direction, model);

			float intensity = 1;
			if (intersection.intersectedTriangle.material->recievesShadow)
				intensity = calculateBrightness(intersection, lightingMode, model, lights);

			Colour colour;
			if (intersection.distance == std::numeric_limits<float>::max()) {
				colour = Colour(0, 0, 0);
			}
			else {
				colour = intersection.intersectedTriangle.GetColour(model, lights, cam, lightingMode,
					intersection.triangleIndex, intersection.intersectionPoint);
				colour.red *= intensity;
				colour.blue *= intensity;
				colour.green *= intensity;
			}
			window.setPixelColour(i, j, colour.getPackedColour());
		}
	}
}