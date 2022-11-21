#include "Utilities.h"

std::vector<float> interpolate(float from, float to, int numberOfValues) {
	float step = (to - from) / (numberOfValues - 1);
	std::vector<float> result(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		result[i] = from + i * step;
	}
	return result;
}

std::vector<CanvasPoint> interpolate(CanvasPoint from, CanvasPoint to, int numberOfValues) {
	std::vector<float> xs = interpolate(from.x, to.x, numberOfValues);
	std::vector<float> tpXs = interpolate(from.textureX, to.textureX, numberOfValues);
	std::vector<float> ys = interpolate(from.y, to.y, numberOfValues);
	std::vector<float> tpYs = interpolate(from.textureY, to.textureY, numberOfValues);
	std::vector<float> depths = interpolate(from.depth, to.depth, numberOfValues);
	std::vector<CanvasPoint> res(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		res[i] = CanvasPoint(std::round(xs[i]), std::round(ys[i]), depths[i]);
		res[i].textureX = tpXs[i];
		res[i].textureY = tpYs[i];
	}
	return res;
}

std::vector<glm::vec3> interpolate(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	std::vector<float> xs = interpolate(from[0], to[0], numberOfValues);
	std::vector<float> ys = interpolate(from[1], to[1], numberOfValues);
	std::vector<float> zs = interpolate(from[2], to[2], numberOfValues);
	std::vector<glm::vec3> res(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		res[i] = glm::vec3(xs[i], ys[i], zs[i]);
	}
	return res;
}

glm::vec3 rotate(glm::vec3 subject, glm::vec3 rotation) {
	glm::mat3 xRotation = { 1, 0, 0,
		0, glm::cos(rotation.x), -glm::sin(rotation.x),
		0, glm::sin(rotation.x), glm::cos(rotation.x) };
	glm::mat3 yRotation = { glm::cos(rotation.y), 0, glm::sin(rotation.y),
				0, 1, 0,
				-glm::sin(rotation.y), 0, glm::cos(rotation.y) };
	glm::mat3 zRotation = { glm::cos(rotation.z), -glm::sin(rotation.z), 0,
		glm::sin(rotation.z), glm::cos(rotation.z), 0,
		0, 0, 1 };
	glm::vec3 result = zRotation * yRotation * xRotation * subject;
	return result;
}

glm::mat3 rotate(glm::mat3 subject, glm::vec3 rotation) {
	glm::mat3 xRotation = { 1, 0, 0,
		0, glm::cos(rotation.x), -glm::sin(rotation.x),
		0, glm::sin(rotation.x), glm::cos(rotation.x) };
	glm::mat3 yRotation = { glm::cos(rotation.y), 0, glm::sin(rotation.y),
				0, 1, 0,
				-glm::sin(rotation.y), 0, glm::cos(rotation.y) };
	glm::mat3 zRotation = { glm::cos(rotation.z), -glm::sin(rotation.z), 0,
		glm::sin(rotation.z), glm::cos(rotation.z), 0,
		0, 0, 1 };
	glm::mat3 result = zRotation * yRotation * xRotation * subject;
	return result;
}

glm::vec3 rotateAbout(glm::vec3 subject, glm::vec3 origin, glm::vec3 rotation) {
	glm::vec3 originToSubject = subject - origin;
	glm::vec3 rotatedOriginToSubject = rotate(originToSubject, rotation);
	glm::vec3 rotatedSubject = rotatedOriginToSubject + origin;
	return rotatedSubject;
}

glm::mat3 lookAt(glm::mat3 subjectOientation, glm::vec3 subjectPosition, glm::vec3 target) {
	glm::vec3 forward = glm::normalize(subjectPosition - target);
	glm::vec3 right = glm::cross(glm::vec3(0, 1, 0), forward);
	glm::vec3 up = glm::cross(forward, right);
	glm::mat3 result = glm::transpose(glm::mat3(right, up, forward));
	return result;
}

glm::vec3 getCenter(std::vector<ModelTriangle> model) {
	glm::vec3 average = glm::vec3(0, 0, 0);
	for (int i = 0; i < model.size(); i++) {
		average += model[i].vertices[0].position;
		average += model[i].vertices[1].position;
		average += model[i].vertices[2].position;
	}
	average /= (model.size() * 3);
	return average;
}

void printVec3(glm::vec3 x) {
	std::cout << "x: " << x.x << ", y: " << x.y << ", z: " << x.z << '\n';
}

std::vector<std::string> split(const std::string& line, char delimiter) {
	auto haystack = line;
	std::vector<std::string> tokens;
	size_t pos;
	while ((pos = haystack.find(delimiter)) != std::string::npos) {
		tokens.push_back(haystack.substr(0, pos));
		haystack.erase(0, pos + 1);
	}
	// Push the remaining chars onto the vector
	tokens.push_back(haystack);
	return tokens;
}

float triangleArea(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
	glm::vec3 AB = v1 - v0;
	glm::vec3 AC = v2 - v0;
	float area = glm::length(glm::cross(AB, AC)) / 2;
	return area;
}