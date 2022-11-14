#include <Utils.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <limits>

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

struct camera {
	glm::vec3 position;
	glm::mat3 orientation;
	float focalLength;
};

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
		result.push_back(ModelTriangle(vertices[faces[i][0] - 1], vertices[faces[i][1] - 1], vertices[faces[i][2] - 1], colours[i]));
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

CanvasPoint getCanvasIntersectionPoint(glm::vec3 vertexPos, DrawingWindow& window, camera cam) {
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

void pointcloudRender(std::vector<ModelTriangle> model, DrawingWindow& window, camera cam) {
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		for (int j = 0; j < 3; j++) { // For each vertex in the triangle...
			CanvasPoint point = getCanvasIntersectionPoint(model[i].vertices[j], window, cam); // Get intersection point...
			window.setPixelColour(point.x, point.y, white); // Set colour
		}
	}
}

void wireframeRender(std::vector<ModelTriangle> model, DrawingWindow& window, camera cam) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(model[i].vertices[0], window, cam);
		CanvasPoint vb = getCanvasIntersectionPoint(model[i].vertices[1], window, cam);
		CanvasPoint vc = getCanvasIntersectionPoint(model[i].vertices[2], window, cam);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
	}
}

void rasterisedRender(std::vector<ModelTriangle> model, DrawingWindow& window, camera cam) {
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
	RayTriangleIntersection result = RayTriangleIntersection(direction,
		std::numeric_limits<float>::max(), target,
		0);;

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

RayTriangleIntersection getClosestIntersection(glm::vec3 startPosition, glm::vec3 direction, std::vector<ModelTriangle> targets) {
	RayTriangleIntersection result = getIntersection(startPosition, direction, targets[0]);

	for (int i = 0; i < targets.size(); i++) {
		RayTriangleIntersection possibleResult = getIntersection(startPosition, direction, targets[i]);
		possibleResult.triangleIndex = i;
		if (possibleResult.distanceFromCamera < result.distanceFromCamera) {
			result = possibleResult;
		}
	}

	return result;
}

void rayTracedRender(std::vector<ModelTriangle> model, DrawingWindow& window, camera cam) {
	for (int i = 0; i < model.size(); i++) {
		for (int j = 0; j < 3; j++) {
			model[i].vertices[j] = cam.orientation * (model[i].vertices[j] - cam.position);
			model[i].vertices[j].x *= window.scale;
			model[i].vertices[j].y *= window.scale;
		}
	}
	for (int i = 0; i < WIDTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {
			glm::vec3 direction = { i - WIDTH/2, HEIGHT/2 - j, -cam.focalLength };
			direction = glm::normalize(direction);
			RayTriangleIntersection intersection = getClosestIntersection(cam.position, direction, model);
			window.setPixelColour(i, j, intersection.intersectedTriangle.colour.getPackedColour());
		}
	}
	std::cout << "Done" << '\n';
}

// MAIN LOOP

void handleEvent(SDL_Event event, DrawingWindow& window, camera* cam) {

	// TODO: Should probably make this a switch
	// TODO: movement should be relative to camera facing, not x, y, z
	// TODO: Ideally we want mouse look
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_u) {
			CanvasTriangle tri = getRandomTriangle(window);
			Colour c = getRandomColour();
			drawStrokedTriangle(tri, c, window, false);
		}
		else if (event.key.keysym.sym == SDLK_f) {
			CanvasTriangle tri = getRandomTriangle(window);
			Colour c = getRandomColour();
			drawFilledTriangle(tri, c, window, false);
			drawStrokedTriangle(tri, Colour(255, 255, 255), window, false);
		}
		else if (event.key.keysym.sym == SDLK_a) { 
			((*cam)).position = (*cam).position + glm::vec3(-CAMERA_MOVE_SPEED, 0, 0); // Translate Camera Left
		}
		else if (event.key.keysym.sym == SDLK_d) { 
			(*cam).position = (*cam).position + glm::vec3(CAMERA_MOVE_SPEED, 0, 0); // Translate Camera Right
		}
		else if (event.key.keysym.sym == SDLK_w) { 
			(*cam).position = (*cam).position + glm::vec3(0, 0, -CAMERA_MOVE_SPEED); // Translate Camera Forward
		}
		else if (event.key.keysym.sym == SDLK_s) { 
			(*cam).position = (*cam).position + glm::vec3(0, 0, CAMERA_MOVE_SPEED); // Translate Camera Back
		}
		else if (event.key.keysym.sym == SDLK_SPACE) { 
			(*cam).position = (*cam).position + glm::vec3(0, CAMERA_MOVE_SPEED, 0); // Translate Camera Up
		}
		else if (event.key.keysym.sym == SDLK_LCTRL) { 
			(*cam).position = (*cam).position + glm::vec3(0, -CAMERA_MOVE_SPEED, 0); // Translate Camera Down
		}
		else if (event.key.keysym.sym == SDLK_LEFT) {
			glm::vec3 rotation = glm::vec3(0, CAMERA_ROTATE_SPEED, 0); // Rotate Camera Left
			(*cam).orientation = rotate((*cam).orientation, rotation);
		}
		else if (event.key.keysym.sym == SDLK_RIGHT) { 
			glm::vec3 rotation = glm::vec3(0, -CAMERA_ROTATE_SPEED, 0); // Rotate Camera Right
			(*cam).orientation = rotate((*cam).orientation, rotation);
		}
		else if (event.key.keysym.sym == SDLK_UP) { 
			glm::vec3 rotation = glm::vec3(CAMERA_ROTATE_SPEED, 0, 0); // Rotate Camera Up
			(*cam).orientation = rotate((*cam).orientation, rotation);
		}
		else if (event.key.keysym.sym == SDLK_DOWN) { 
			glm::vec3 rotation = glm::vec3(-CAMERA_ROTATE_SPEED, 0, 0); // Rotate Camera Down
			(*cam).orientation = rotate((*cam).orientation, rotation);
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

	camera mainCamera;
	mainCamera.focalLength = 2;
	mainCamera.position = glm::vec3(0, 0, 4);
	mainCamera.orientation = glm::mat3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	std::string mtlFilepath = "cornell-box.mtl";
	std::unordered_map<std::string, Colour> palette = readMTL(mtlFilepath);
	std::string objFilepath = "cornell-box.obj";
	std::vector<ModelTriangle> cornellBox = readOBJ(objFilepath, palette, 0.35);

	rayTracedRender(cornellBox, window, mainCamera);
	//rasterisedRender(cornellBox, window, mainCamera);

	while (true) {
		//window.clearPixels();

		if (window.pollForInputEvents(event)) handleEvent(event, window, &mainCamera);

		// draw() {


		// }

		//mainCamera.position = rotateAbout(mainCamera.position, glm::vec3(0, 0, 0), glm::vec3(0, -CAMERA_MOVE_SPEED / 10, 0));
		//mainCamera.orientation = lookAt(mainCamera.orientation, mainCamera.position, glm::vec3(0, 0, 0));

		window.renderFrame();
	}
}