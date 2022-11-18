#include <Utils.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <limits>
#include <algorithm>

#include <CanvasTriangle.h>
#include <CanvasPoint.h>
#include <DrawingWindow.h>
#include <Colour.h>
#include <TextureMap.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>

#include <glm/vec3.hpp>
#include <glm/glm.hpp> // There's more stuff here than I need

#include <AustinUtils.h>

#define WIDTH 320
#define HEIGHT 240
#define IMAGE_PLANE_SCALE 180
#define CAMERA_MOVE_SPEED 0.15
#define CAMERA_ROTATE_SPEED 0.01
#define PI 3.14159265358979323846264338327950288

enum RenderMode {
	POINTCLOUD,
	WIREFRAME,
	RASTERISED,
	RAYTRACED
};

enum LightingMode {
	HARD,
	PROXIMITY,
	INCIDENCE,
	SPECULAR,
	AMBIENT
};

struct Camera {
	glm::vec3 position;
	glm::mat3 orientation;
	float focalLength;
};

struct RendererState {
	RenderMode renderMode;
	LightingMode lightingMode;
	bool orbiting;
};

void printVec3(glm::vec3 x) {
	std::cout << "x: " << x.x << ", y: " << x.y << ", z: " << x.z << '\n';
}

// FILE PARSERS

std::unordered_map<std::string, Colour> readMTL(std::string& filepath) {
	std::unordered_map<std::string, Colour> result = {};
	std::ifstream inputStream(filepath, std::ifstream::binary);
	std::string nextLine;
	while (!inputStream.eof()) {
		std::getline(inputStream, nextLine); // Name
		auto lineContents = split(nextLine, ' ');
		std::string name = lineContents[1];
		std::getline(inputStream, nextLine); // Value
		lineContents = split(nextLine, ' ');
		int r = std::round(std::stof(lineContents[1]) * 255);
		int g = std::round(std::stof(lineContents[2]) * 255);
		int b = std::round(std::stof(lineContents[3]) * 255);
		Colour colour = Colour(r, g, b);
		result[name] = colour;
		std::getline(inputStream, nextLine); // Empty
	}
	return result;
}

std::vector<ModelTriangle> readOBJ(std::string& filepath, std::unordered_map<std::string, Colour> palette, float scaleFactor = 1) {

	std::ifstream inputStream(filepath, std::ifstream::binary);
	std::string nextLine;
	std::vector<glm::vec3> vertices = {};
	std::vector<std::array<int, 3>> faces = {};
	std::vector<Colour> colours = {};
	std::vector<ModelTriangle> result = {};

	std::getline(inputStream, nextLine); // The mtl bit at the top - should probably use this to reference mtl file rather than pass it in

	std::getline(inputStream, nextLine); // Empty

	while (!inputStream.eof()) {

		// Each loop is one block of stuff.

		std::getline(inputStream, nextLine); // Face Name

		std::getline(inputStream, nextLine); // Colour
		auto lineContents = split(nextLine, ' ');
		Colour c = palette[lineContents[1]];

		// Get vertices
		std::getline(inputStream, nextLine);
		while (nextLine[0] == 'v') {
			auto lineContents = split(nextLine, ' ');
			float x = std::stof(lineContents[1]);
			float y = std::stof(lineContents[2]);
			float z = std::stof(lineContents[3]);
			vertices.push_back(glm::vec3(x, y, z));
			std::getline(inputStream, nextLine);
		}

		// Get faces
		while (nextLine[0] == 'f') {
			auto lineContents = split(nextLine, ' ');
			lineContents[1][lineContents[1].size() - 1] = '\0';
			lineContents[2][lineContents[2].size() - 1] = '\0';
			lineContents[3][lineContents[3].size() - 2] = '\0';
			int faceIndexA = std::stoi(lineContents[1]);
			int faceIndexB = std::stoi(lineContents[2]);
			int faceIndexC = std::stoi(lineContents[3]);
			faces.push_back({ faceIndexA, faceIndexB, faceIndexC });
			colours.push_back(c);
			std::getline(inputStream, nextLine);
		}
	}

	// Model is flipped horizontally because our coordinate space is different.
	// Scale factor is applied.
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].x *= scaleFactor;
		vertices[i].y *= scaleFactor;
		vertices[i].z *= scaleFactor;
	}

	// Go through the data we've collected and create Model Triangles.
	for (int i = 0; i < faces.size(); i++) {
		ModelTriangle triangle = ModelTriangle(vertices[faces[i][0] - 1], vertices[faces[i][1] - 1], vertices[faces[i][2] - 1], colours[i]);
		glm::vec3 v0toV1 = triangle.vertices[1] - triangle.vertices[0];
		glm::vec3 v0toV2 = triangle.vertices[2] - triangle.vertices[0];
		glm::vec3 normal = glm::cross(v0toV1, v0toV2);
		triangle.normal = glm::normalize(normal);
		result.push_back(triangle);
	}

	inputStream.close();
	return result;
}

// INTERPOLATION FUNCTIONS

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
	std::vector<float> tpXs = interpolate(from.texturePoint.x, to.texturePoint.x, numberOfValues);
	std::vector<float> ys = interpolate(from.y, to.y, numberOfValues);
	std::vector<float> tpYs = interpolate(from.texturePoint.y, to.texturePoint.y, numberOfValues);
	std::vector<float> depths = interpolate(from.depth, to.depth, numberOfValues);
	std::vector<CanvasPoint> res(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		res[i] = CanvasPoint(std::round(xs[i]), std::round(ys[i]), depths[i]);
		res[i].texturePoint = TexturePoint(tpXs[i], tpYs[i]);
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

// RENDERING UTILS

std::vector<CanvasPoint> getLine(CanvasPoint from, CanvasPoint to) {
	std::vector<CanvasPoint> result;
	if ((from.x == to.x) && (from.y == to.y)) {
		result = { from };
	}
	else {
		float xDiff = to.x - from.x;
		float yDiff = to.y - from.y;
		int stepNum = std::max(abs(xDiff), abs(yDiff)) + 1;
		result = interpolate(from, to, stepNum);
	}
	return result;
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 vertexPos, DrawingWindow& window, Camera cam) {
	glm::vec3 cameraSpaceVertex = cam.orientation * (vertexPos - cam.position);
	cameraSpaceVertex.x *= window.scale;
	cameraSpaceVertex.y *= window.scale;

	// Formula taken from the worksheet.
	float u = WIDTH / 2 - (cam.focalLength * (cameraSpaceVertex.x / cameraSpaceVertex.z));
	float v = cam.focalLength * (cameraSpaceVertex.y / cameraSpaceVertex.z) + (window.height / 2);
	return CanvasPoint(u, v, cameraSpaceVertex.z);
}

// ROTATION

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
	glm::mat3 yRotation = {glm::cos(rotation.y), 0, glm::sin(rotation.y),
				0, 1, 0,
				-glm::sin(rotation.y), 0, glm::cos(rotation.y)};
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

// Gets the center of a model.
glm::vec3 getCenter(std::vector<ModelTriangle> model) {
	glm::vec3 average = glm::vec3(0, 0, 0);
	for (int i = 0; i < model.size(); i++) {
		average += model[i].vertices[0];
		average += model[i].vertices[1];
		average += model[i].vertices[2];
	}
	average /= (model.size() * 3);
	return average;
}

// DRAWING PRIMATIVES

void drawLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow& window, bool useDepth = true) {
	std::vector<CanvasPoint> line = getLine(from, to);
	uint32_t c = colour.getPackedColour();
	for (int i = 0; i < line.size(); i++) {
		useDepth ? window.setPixelColour(line[i].x, line[i].y, line[i].depth, c) : window.setPixelColour(line[i].x, line[i].y, c);
	}
}

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true) {
	drawLine(triangle.v0(), triangle.v1(), colour, window, useDepth);
	drawLine(triangle.v1(), triangle.v2(), colour, window, useDepth);
	drawLine(triangle.v0(), triangle.v2(), colour, window, useDepth);
}

void drawFlatBottomedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
	float depthV0 = 1 / triangle.v0().depth;
	float depthV1 = 1 / triangle.v1().depth;
	float depthV2 = 1 / triangle.v2().depth;
	// Assume triangle vertices are sorted by y value in ascending order.
	int height = std::abs(triangle.v0().y - triangle.v1().y);
	float xStep1 = (float)(triangle.v1().x - triangle.v0().x) / height;
	float xStep2 = (float)(triangle.v2().x - triangle.v0().x) / height;
	float depthStep1 = (depthV1 - depthV0) / height;
	float depthStep2 = (depthV2 - depthV0) / height;

	// Could use v1 or v2 here for the condition, both should have the same height.
	for (int i = triangle.v0().y; i <= triangle.v1().y; i++) {
		float x1 = (i - triangle.v0().y) * xStep1 + triangle.v0().x;
		float depth1 = (i - triangle.v0().y) * depthStep1 + depthV0;
		float x2 = (i - triangle.v0().y) * xStep2 + triangle.v0().x;
		float depth2 = (i - triangle.v0().y) * depthStep2 + depthV0;
		CanvasPoint p1 = CanvasPoint(std::round(x1), i, depth1);
		CanvasPoint p2 = CanvasPoint(std::round(x2), i, depth2);
		drawLine(p1, p2, colour, window, useDepth);
	}
}

void drawFlatToppedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
	float depthV0 = 1 / triangle.v0().depth;
	float depthV1 = 1 / triangle.v1().depth;
	float depthV2 = 1 / triangle.v2().depth;
	// Assume triangle vertices are sorted by y value in ascending order.
	int height = std::abs(triangle.v2().y - triangle.v0().y);
	float xStep1 = (float)(triangle.v0().x - triangle.v2().x) / height;
	float xStep2 = (float)(triangle.v1().x - triangle.v2().x) / height;
	float depthStep1 = (depthV0 - depthV2) / height;
	float depthStep2 = (depthV1 - depthV2) / height;

	// Could use v0 or v1 here for the condition, both should have the same height.
	for (int i = triangle.v2().y; i >= triangle.v0().y; i--) {
		float x1 = (triangle.v2().y - i) * xStep1 + triangle.v2().x;
		float depth1 = (triangle.v2().y - i) * depthStep1 + depthV2;
		float x2 = (triangle.v2().y - i) * xStep2 + triangle.v2().x;
		float depth2 = (triangle.v2().y - i) * depthStep2 + depthV2;
		CanvasPoint p1 = CanvasPoint(std::round(x1), i, depth1);
		CanvasPoint p2 = CanvasPoint(std::round(x2), i, depth2);
		drawLine(p1, p2, colour, window, useDepth);
	}
}

// Even though we use std::swap, this doesn't seem to sort triangle as a side effect.
void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true) {
	// Vertices are sorted in ascending y order.
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());
	if (triangle.v1().y > triangle.v2().y) std::swap(triangle.v1(), triangle.v2());
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());

	if (triangle.v0().y == triangle.v1().y) {
		drawFlatToppedTriangle(triangle, colour, window, useDepth);
	}
	else if (triangle.v1().y == triangle.v2().y) {
		drawFlatBottomedTriangle(triangle, colour, window, useDepth);
	}
	else {
		float midPointX = triangle.v0().x + ((triangle.v1().y - triangle.v0().y) * (triangle.v2().x - triangle.v0().x)) / (triangle.v2().y - triangle.v0().y);
		float midPointDepth = triangle.v0().depth + ((triangle.v1().y - triangle.v0().y) * (triangle.v2().depth - triangle.v0().depth)) / (triangle.v2().y - triangle.v0().y);
		CanvasPoint midPoint = CanvasPoint(std::round(midPointX), triangle.v1().y, midPointDepth);
		drawFlatBottomedTriangle(CanvasTriangle(triangle.v0(), triangle.v1(), midPoint), colour, window, useDepth);
		drawFlatToppedTriangle(CanvasTriangle(triangle.v1(), midPoint, triangle.v2()), colour, window, useDepth);
	}
}

void drawTexturedTriangle(CanvasTriangle triangle, TextureMap texture, DrawingWindow& window) {
	// Vertices are sorted such that v0 has the lowest y value, meaning it's the highest.
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());
	if (triangle.v1().y > triangle.v2().y) std::swap(triangle.v1(), triangle.v2());
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());

	std::vector<CanvasPoint> topToMiddle = getLine(triangle.v0(), triangle.v1());
	std::vector<CanvasPoint> middleToBottom = getLine(triangle.v1(), triangle.v2());
	std::vector<CanvasPoint> topToBottom = getLine(triangle.v0(), triangle.v2());

	CanvasPoint middle;
	for (int i = 0; i < topToBottom.size(); i++) {
		if (topToBottom[i].y == triangle.v1().y) middle = topToBottom[i];
	}

	for (int i = triangle.v0().y; i < middle.y; i++) {

		CanvasPoint point1;
		for (int j = 0; j < topToMiddle.size(); j++) {
			if (topToMiddle[j].y == i) point1 = topToMiddle[j];
		}
		CanvasPoint point2;
		for (int j = 0; j < topToBottom.size(); j++) {
			if (topToBottom[j].y == i) point2 = topToBottom[j];
		}

		std::vector<CanvasPoint> line = getLine(point1, point2);
		for (int j = 0; j < line.size(); j++) {
			int textureIndex = (line[j].texturePoint.y * texture.width) + line[j].texturePoint.x;
			window.setPixelColour(line[j].x, line[j].y, texture.pixels[textureIndex]);
		}
	}
	for (int i = middle.y; i <= triangle.v2().y; i++) {

		CanvasPoint point1;
		for (int j = 0; j < middleToBottom.size(); j++) {
			if (middleToBottom[j].y == i) point1 = middleToBottom[j];
		}
		CanvasPoint point2;
		for (int j = 0; j < topToBottom.size(); j++) {
			if (topToBottom[j].y == i) point2 = topToBottom[j];
		}

		std::vector<CanvasPoint> line = getLine(point1, point2);
		for (int j = 0; j < line.size(); j++) {
			int textureIndex = (line[j].texturePoint.y * texture.width) + line[j].texturePoint.x;
			window.setPixelColour(line[j].x, line[j].y, texture.pixels[textureIndex]);
		}
	}
}

void pointcloudRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		for (int j = 0; j < 3; j++) { // For each vertex in the triangle...
			CanvasPoint point = getCanvasIntersectionPoint(model[i].vertices[j], window, cam); // Get intersection point...
			window.setPixelColour(point.x, point.y, white); // Set colour
		}
	}
}

void wireframeRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(model[i].vertices[0], window, cam);
		CanvasPoint vb = getCanvasIntersectionPoint(model[i].vertices[1], window, cam);
		CanvasPoint vc = getCanvasIntersectionPoint(model[i].vertices[2], window, cam);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
	}
}

void rasterisedRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(model[i].vertices[0], window, cam);
		CanvasPoint vb = getCanvasIntersectionPoint(model[i].vertices[1], window, cam);
		CanvasPoint vc = getCanvasIntersectionPoint(model[i].vertices[2], window, cam);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawFilledTriangle(triangle, model[i].colour, window);
	}
}

// RAYTRACING

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

float incidenceLighting(RayTriangleIntersection intersection, glm::vec3 light) {
	glm::vec3 pointToLight = glm::normalize(light - intersection.intersectionPoint);
	float similarity = std::max(glm::dot(pointToLight, intersection.intersectedTriangle.normal), 0.0f);
	return similarity;
}

float specularLighting(RayTriangleIntersection intersection, glm::vec3 light, int specularExponent = 128) {
	glm::vec3 unitLightToPoint = -glm::normalize(light - intersection.intersectionPoint);
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 reflection = unitLightToPoint - (2.0f * normal * glm::dot(unitLightToPoint, normal));

	float reflectionSimilarity = -glm::dot(glm::normalize(reflection), glm::normalize(intersection.intersectionPoint));
	reflectionSimilarity = reflectionSimilarity < 0 ? 0 : reflectionSimilarity;
	reflectionSimilarity = std::pow(reflectionSimilarity, specularExponent);
	return reflectionSimilarity;
}

float ambientLighting(float currentIntensity, float addition = 0.2) {
	return std::min(currentIntensity + addition, 1.0f);
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

	for (int i = 0; i < WIDTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {

			glm::vec3 direction = { (i - WIDTH/2) / window.scale, (HEIGHT/2 - j) / window.scale, -cam.focalLength};
			direction = glm::normalize(direction);
			RayTriangleIntersection intersection = getClosestIntersection(glm::vec3(0, 0, 0), direction, model);

			float intensity;
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
					default:
						intensity = 1;
						break;
				}
			}
			Colour colour = Colour(intersection.intersectedTriangle.colour.red * intensity,
				intersection.intersectedTriangle.colour.green * intensity,
				intersection.intersectedTriangle.colour.blue * intensity);
			window.setPixelColour(i, j, colour.getPackedColour());
		}
	}
}

// MAIN LOOP

void handleEvent(SDL_Event event, DrawingWindow& window, Camera* cam, RendererState* state) {
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
			case SDLK_a:
				((*cam)).position = (*cam).position + glm::vec3(-CAMERA_MOVE_SPEED, 0, 0);
				break;
			case SDLK_d:
				(*cam).position = (*cam).position + glm::vec3(CAMERA_MOVE_SPEED, 0, 0);
				break;
			case SDLK_w:
				(*cam).position = (*cam).position + glm::vec3(0, 0, -CAMERA_MOVE_SPEED);
				break;
			case SDLK_s:
				(*cam).position = (*cam).position + glm::vec3(0, 0, CAMERA_MOVE_SPEED);
				break;
			case SDLK_SPACE:
				(*cam).position = (*cam).position + glm::vec3(0, CAMERA_MOVE_SPEED, 0);
				break;
			case SDLK_LCTRL:
				(*cam).position = (*cam).position + glm::vec3(0, -CAMERA_MOVE_SPEED, 0);
				break;
			case SDLK_LEFT:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(0, CAMERA_ROTATE_SPEED, 0));
				break;
			case SDLK_RIGHT:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(0, -CAMERA_ROTATE_SPEED, 0));
				break;
			case SDLK_UP:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(CAMERA_ROTATE_SPEED, 0, 0));
				break;
			case SDLK_DOWN:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(-CAMERA_ROTATE_SPEED, 0, 0));
				break;
			case SDLK_o:
				(*state).orbiting = !(*state).orbiting;
				break;
			case SDLK_1:
				(*state).renderMode = POINTCLOUD;
				break;
			case SDLK_2:
				(*state).renderMode = WIREFRAME;
				break;
			case SDLK_3:
				(*state).renderMode = RASTERISED;
				break;
			case SDLK_4:
				(*state).renderMode = RAYTRACED;
				break;
			case SDLK_5:
				(*state).lightingMode = HARD;
				break;
			case SDLK_6:
				(*state).lightingMode = PROXIMITY;
				break;
			case SDLK_7:
				(*state).lightingMode = SPECULAR;
				break;
			case SDLK_8:
				(*state).lightingMode = AMBIENT;
				break;
			default:
				break;
		}
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char* argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, IMAGE_PLANE_SCALE, false);
	SDL_Event event;

	RendererState state;
	state.renderMode = RAYTRACED;
	state.orbiting = false;
	state.lightingMode = AMBIENT;

	Camera mainCamera;
	mainCamera.focalLength = 2;
	mainCamera.position = glm::vec3(0, 0, 4);
	mainCamera.orientation = glm::mat3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	glm::vec3 lightPosition = {0.5, 0.8, 1.0};

	std::string mtlFilepath = "cornell-box.mtl";
	std::unordered_map<std::string, Colour> palette = readMTL(mtlFilepath);
	std::string objFilepath = "cornell-box.obj";
	std::vector<ModelTriangle> cornellBox = readOBJ(objFilepath, palette, 0.35);

	while (true) {
		if (window.pollForInputEvents(event)) handleEvent(event, window, &mainCamera, &state);

		// draw() {
		window.clearPixels();

		switch (state.renderMode) {
			case POINTCLOUD:
				pointcloudRender(cornellBox, window, mainCamera);
				break;
			case WIREFRAME:
				wireframeRender(cornellBox, window, mainCamera);
				break;
			case RASTERISED:
				rasterisedRender(cornellBox, window, mainCamera);
				break;
			case RAYTRACED:
				rayTracedRender(cornellBox, lightPosition, window, mainCamera, state.lightingMode);
				break;
		}

		window.renderFrame();
		// }

		if (state.orbiting) {
			mainCamera.position = rotateAbout(mainCamera.position, glm::vec3(0, 0, 0), glm::vec3(0, -CAMERA_MOVE_SPEED / 10, 0));
			mainCamera.orientation = lookAt(mainCamera.orientation, mainCamera.position, glm::vec3(0, 0, 0));
		}

	}
}