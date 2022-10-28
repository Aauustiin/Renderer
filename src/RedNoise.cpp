#include <Utils.h>
#include <fstream>
#include <vector>
#include <unordered_map>

#include <CanvasTriangle.h>
#include <CanvasPoint.h>
#include <DrawingWindow.h>
#include <Colour.h>
#include <TextureMap.h>
#include <ModelTriangle.h>

#include <glm/vec3.hpp>
#include <glm/glm.hpp> // There's more stuff here than I need

#include <AustinUtils.h>
#include <chrono>

#define WIDTH 320
#define HEIGHT 240
#define IMAGE_PLANE_SCALE 180
#define CAMERA_MOVE_SPEED 0.05
#define CAMERA_ROTATE_SPEED 0.01

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true);
void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true);

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
		vertices[i].x *= -scaleFactor;
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

// TODO: Puts extra pixel at from
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

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPos, glm::vec3 vertexPos, float focalLength, DrawingWindow& window, glm::mat3 cameraOrientation) {
	glm::vec3 cameraSpaceVertex = cameraOrientation * (vertexPos - cameraPos);
	cameraSpaceVertex.x *= window.scale;
	cameraSpaceVertex.y *= window.scale;

	// Formula taken from the worksheet.
	float u = focalLength * (cameraSpaceVertex.x / cameraSpaceVertex.z) + (window.width / 2);
	float v = focalLength * (cameraSpaceVertex.y / cameraSpaceVertex.z) + (window.height / 2);
	return CanvasPoint(u, v, cameraSpaceVertex.z);
}

// WORKSPACE

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
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0)); // TODO: Inverts image horizontally
	glm::vec3 up = glm::cross(right, forward);
	return glm::mat3(right, up, forward);
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
	uint32_t c = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
	for (int i = 0; i < line.size(); i++) {
		useDepth ? window.setPixelColour(line[i].x, line[i].y, line[i].depth, c) : window.setPixelColour(line[i].x, line[i].y, c);
	}
}

void pointcloudRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow& window, glm::mat3 cameraOrientation) {
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		for (int j = 0; j < 3; j++) { // For each vertex in the triangle...
			CanvasPoint point = getCanvasIntersectionPoint(cameraPos, model[i].vertices[j], focalLength, window, cameraOrientation); // Get intersection point...
			window.setPixelColour(point.x, point.y, white); // Set colour
		}
	}
}

void wireframeRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow& window, glm::mat3 cameraOrientation) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(cameraPos, model[i].vertices[0], focalLength, window, cameraOrientation);
		CanvasPoint vb = getCanvasIntersectionPoint(cameraPos, model[i].vertices[1], focalLength, window, cameraOrientation);
		CanvasPoint vc = getCanvasIntersectionPoint(cameraPos, model[i].vertices[2], focalLength, window, cameraOrientation);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
	}
}

void rasterisedRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow& window, glm::mat3 cameraOrientation) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(cameraPos, model[i].vertices[0], focalLength, window, cameraOrientation);
		CanvasPoint vb = getCanvasIntersectionPoint(cameraPos, model[i].vertices[1], focalLength, window, cameraOrientation);
		CanvasPoint vc = getCanvasIntersectionPoint(cameraPos, model[i].vertices[2], focalLength, window, cameraOrientation);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawFilledTriangle(triangle, model[i].colour, window);
		//drawStrokedTriangle(triangle, model[i].colour, window);
	}
}

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
	drawLine(triangle.v0(), triangle.v1(), colour, window, useDepth);
	drawLine(triangle.v1(), triangle.v2(), colour, window, useDepth);
	drawLine(triangle.v0(), triangle.v2(), colour, window, useDepth);
}

void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
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
		drawLine(point1, point2, colour, window, useDepth);
	}
	for (int i = middle.y; i <= triangle.v2().y; i++) {
		CanvasPoint point1;
		for (int j = 0; j < middleToBottom.size(); j++) {
			if (middleToBottom[j].y == i) {
				point1 = middleToBottom[j];
				break;
			}
		}
		CanvasPoint point2;
		for (int j = 0; j < topToBottom.size(); j++) {
			if (topToBottom[j].y == i) point2 = topToBottom[j];
		}
		drawLine(point1, point2, colour, window, useDepth);
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

// MAIN LOOP

void handleEvent(SDL_Event event, DrawingWindow& window,
	glm::vec3* cameraPosPtr, glm::mat3* cameraOrientationPtr,
	long long deltaTime) {

	// TODO: How do I get these inputs frame rate independant?
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
			//drawStrokedTriangle(tri, Colour(255, 255, 255), window, false);
		}
		else if (event.key.keysym.sym == SDLK_a) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(CAMERA_MOVE_SPEED, 0, 0); // Translate Camera Left
		}
		else if (event.key.keysym.sym == SDLK_d) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(-CAMERA_MOVE_SPEED, 0, 0); // Translate Camera Right
		}
		else if (event.key.keysym.sym == SDLK_w) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(0, 0, -CAMERA_MOVE_SPEED); // Translate Camera Forward
		}
		else if (event.key.keysym.sym == SDLK_s) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(0, 0, CAMERA_MOVE_SPEED); // Translate Camera Back
		}
		else if (event.key.keysym.sym == SDLK_SPACE) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(0, CAMERA_MOVE_SPEED, 0); // Translate Camera Up
		}
		else if (event.key.keysym.sym == SDLK_LCTRL) { 
			*cameraPosPtr = *cameraPosPtr + glm::vec3(0, -CAMERA_MOVE_SPEED, 0); // Translate Camera Down
		}
		else if (event.key.keysym.sym == SDLK_LEFT) {
			glm::vec3 rotation = glm::vec3(0, -CAMERA_ROTATE_SPEED, 0); // Rotate Camera Left
			*cameraOrientationPtr = rotate(*cameraOrientationPtr, rotation);
		}
		else if (event.key.keysym.sym == SDLK_RIGHT) { 
			glm::vec3 rotation = glm::vec3(0, CAMERA_ROTATE_SPEED, 0); // Rotate Camera Right
			*cameraOrientationPtr = rotate(*cameraOrientationPtr, rotation);
		}
		else if (event.key.keysym.sym == SDLK_UP) { 
			glm::vec3 rotation = glm::vec3(CAMERA_ROTATE_SPEED, 0, 0); // Rotate Camera Up
			*cameraOrientationPtr = rotate(*cameraOrientationPtr, rotation);
		}
		else if (event.key.keysym.sym == SDLK_DOWN) { 
			glm::vec3 rotation = glm::vec3(-CAMERA_ROTATE_SPEED, 0, 0); // Rotate Camera Down
			*cameraOrientationPtr = rotate(*cameraOrientationPtr, rotation);
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

	glm::vec3 cameraPosition = { 0, 0, 4 };
	glm::vec3* cameraPosPtr = &cameraPosition;
	glm::mat3 cameraOrientation = glm::mat3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);
	glm::mat3* cameraOrientationPtr = &cameraOrientation;

	float focalLength = 2;

	std::string mtlFilepath = "cornell-box.mtl";
	std::unordered_map<std::string, Colour> palette = readMTL(mtlFilepath);
	std::string objFilepath = "cornell-box.obj";
	std::vector<ModelTriangle> cornellBox = readOBJ(objFilepath, palette, 0.35);
	glm::vec3 cornellBoxCenter = getCenter(cornellBox);
	
	
	std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point tempTime;
	/*
	Note: This isn't always long long, it's type is actually std::chrono::milliseconds::rep, but in most c++ implementations this is
	equivalent to long long.
	*/
	long long deltaTime;
	
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	CanvasPoint p1 = CanvasPoint(2, 1);
	CanvasPoint p2 = CanvasPoint(0, 3);
	CanvasPoint p3 = CanvasPoint(4, 3);
	CanvasTriangle tri = CanvasTriangle(p1, p2, p3);
	drawFilledTriangle(tri, getRandomColour(), window, false);
	window.setPixelColour(p1.x, p1.y, white);
	window.setPixelColour(p2.x, p2.y, white);
	window.setPixelColour(p3.x, p3.y, white);

	while (true) {

		tempTime = std::chrono::high_resolution_clock::now();;
		// At this point, currentTime will be the time recorded last frame.
		deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(tempTime - currentTime).count() / 1000000;
		// Delta time stuff doesn't work quite how I'd like it to though, revisit later.
		// We probably want delta time as a fraction of a second, that way it's easy to say things like, I want the camera to orbit the box once every
		// Two seconds
		currentTime = tempTime;

		if (window.pollForInputEvents(event)) handleEvent(event, window, cameraPosPtr, cameraOrientationPtr, deltaTime);

		// draw() {

		rasterisedRender(cornellBox, cameraPosition, focalLength, window, cameraOrientation);

		// }

		//cameraPosition = rotateAbout(cameraPosition, getCenter(cornellBox), glm::vec3(0, CAMERA_MOVE_SPEED/10, 0));
		//cameraOrientation = lookAt(cameraOrientation, cameraPosition, glm::vec3(0, 0, 0));

		window.renderFrame();
		//window.clearPixels();
	}
}

// TODO: When rotating camera pos about origin, random blue line is drawn in the top right when the box is in the center of the screen?
// TODO: Shouldn't be able to see part of the ceilling/Floor through the wall when looking at the box from the side
// TODO: Bottom line of the floor isn't being coloured in