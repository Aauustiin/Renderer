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

#define WIDTH 320
#define HEIGHT 240

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window);
void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window);

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPos, glm::vec3 vertexPos, float focalLength, DrawingWindow &window) {
	glm::vec3 cameraSpaceVertex = vertexPos - cameraPos;
	cameraSpaceVertex.x *= 180.0f;
	cameraSpaceVertex.y *= 180.0f;

	// Formula taken from the worksheet.
	float u = focalLength * (cameraSpaceVertex.x / cameraSpaceVertex.z) + (window.width / 2);
	float v = focalLength * (cameraSpaceVertex.y / cameraSpaceVertex.z) + (window.height / 2);
	return CanvasPoint(std::round(u), std::round(v));
}

void pointcloudRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow &window) {
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;
	
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		for (int j = 0; j < 3; j++) { // For each vertex in the triangle...
			CanvasPoint point = getCanvasIntersectionPoint(cameraPos, model[i].vertices[j], focalLength, window); // Get intersection point...
			window.setPixelColour(point.x, point.y, white); // Set colour
		}
	}
}

void wireframeRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow& window) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(cameraPos, model[i].vertices[0], focalLength, window);
		CanvasPoint vb = getCanvasIntersectionPoint(cameraPos, model[i].vertices[1], focalLength, window);
		CanvasPoint vc = getCanvasIntersectionPoint(cameraPos, model[i].vertices[2], focalLength, window);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
	}
}

void rasterisedRender(std::vector<ModelTriangle> model, glm::vec3 cameraPos, float focalLength, DrawingWindow& window) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(cameraPos, model[i].vertices[0], focalLength, window);
		CanvasPoint vb = getCanvasIntersectionPoint(cameraPos, model[i].vertices[1], focalLength, window);
		CanvasPoint vc = getCanvasIntersectionPoint(cameraPos, model[i].vertices[2], focalLength, window);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawFilledTriangle(triangle, model[i].colour, window);
	}
}

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
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].x *= -scaleFactor;
		vertices[i].y *= scaleFactor;
		vertices[i].z *= scaleFactor;
	}

	// Scale model by the scale factor.
	//for (int i = 0; i < vertices.size(); i++) {
	//	vertices[i] *= scaleFactor;
	//}

	// Go through the data we've collected and create Model Triangles.
	for (int i = 0; i < faces.size(); i++) {
		result.push_back(ModelTriangle(vertices[faces[i][0] - 1], vertices[faces[i][1] - 1], vertices[faces[i][2] - 1], colours[i]));
	}

	inputStream.close();
	return result;
}

std::vector<float> interpolateFloat(float from, float to, int numberOfValues) {
	float step = (to - from) / (numberOfValues - 1);
	std::vector<float> result(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		result[i] = from + i * step;
	}
	return result;
}

std::vector<CanvasPoint> interpolateCanvasPoint(CanvasPoint from, CanvasPoint to, int numberOfValues) {
	std::vector<float> xs = interpolateFloat(from.x, to.x, numberOfValues);
	std::vector<float> tpXs = interpolateFloat(from.texturePoint.x, to.texturePoint.x, numberOfValues);
	std::vector<float> ys = interpolateFloat(from.y, to.y, numberOfValues);
	std::vector<float> tpYs = interpolateFloat(from.texturePoint.y, to.texturePoint.y, numberOfValues);
	std::vector<CanvasPoint> res(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		res[i] = CanvasPoint(xs[i], ys[i]);
		res[i].texturePoint = TexturePoint(std::round(tpXs[i]), std::round(tpYs[i]));
	}
	return res;
}

std::vector<glm::vec3> interpolateVec3(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	std::vector<float> xs = interpolateFloat(from[0], to[0], numberOfValues);
	std::vector<float> ys = interpolateFloat(from[1], to[1], numberOfValues);
	std::vector<float> zs = interpolateFloat(from[2], to[2], numberOfValues);
	std::vector<glm::vec3> res(numberOfValues);
	for (int i = 0; i < numberOfValues; i++) {
		res[i] = glm::vec3(xs[i], ys[i], zs[i]);
	}
	return res;
}

std::vector<CanvasPoint> getLine(CanvasPoint from, CanvasPoint to) {
	std::vector<CanvasPoint> result;
	if ((from.x == to.x) && (from.y == to.y)) {
		result = { from };
	}
	else {
		float xDiff = to.x - from.x;
		float yDiff = to.y - from.y;
		int stepNum = std::max(abs(xDiff), abs(yDiff)) + 1;
		result = interpolateCanvasPoint(from, to, stepNum);
		for (int i = 0; i < result.size(); i++) {
			result[i].x = std::round(result[i].x);
			result[i].y = std::round(result[i].y);
		}
	}
	return result;
}

void drawLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow& window) {
	std::vector<CanvasPoint> line = getLine(from, to);
	uint32_t c = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
	for (int i = 0; i < line.size(); i++) {
		window.setPixelColour(line[i].x, line[i].y, c);
	}
}

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window) {
	drawLine(triangle.v0(), triangle.v1(), colour, window);
	drawLine(triangle.v1(), triangle.v2(), colour, window);
	drawLine(triangle.v0(), triangle.v2(), colour, window);
}

void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window) {
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
		drawLine(point1, point2, colour, window);
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
		drawLine(point1, point2, colour, window);
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

void drawTexturedTriangleExample(DrawingWindow& window) {
	CanvasPoint a = CanvasPoint(160, 10);
	a.texturePoint = TexturePoint(195, 5);
	CanvasPoint b = CanvasPoint(300, 230);
	b.texturePoint = TexturePoint(395, 380);
	CanvasPoint c = CanvasPoint(10, 150);
	c.texturePoint = TexturePoint(65, 330);
	CanvasTriangle triangle = CanvasTriangle(a, b, c);
	TextureMap texture = TextureMap("texture.ppm");
	drawTexturedTriangle(triangle, texture, window);
	drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
}

void drawRedNoise(DrawingWindow& window) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = rand() % 256;
			float green = 0.0;
			float blue = 0.0;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

void draw1dGreyscaleLerp(DrawingWindow& window) {
	window.clearPixels();
	std::vector<float> values = interpolateFloat(0, 255, window.width);
	for (size_t x = 0; x < window.width; x++) {
		uint32_t colour = (255 << 24) + (int(values[x]) << 16) + (int(values[x]) << 8) + int(values[x]);
		for (size_t y = 0; y < window.height; y++) {
			window.setPixelColour(x, y, colour);
		}
	}
}

void draw2dColourLerp(DrawingWindow& window) {
	window.clearPixels();

	glm::vec3 topLeft(255, 0, 0);        // red 
	glm::vec3 topRight(0, 0, 255);       // blue 
	glm::vec3 bottomRight(0, 255, 0);    // green 
	glm::vec3 bottomLeft(255, 255, 0);   // yellow

	std::vector<glm::vec3> leftColumn = interpolateVec3(topLeft, bottomLeft, window.height);
	std::vector<glm::vec3> rightColumn = interpolateVec3(topRight, bottomRight, window.height);

	for (size_t y = 0; y < window.height; y++) {
		std::vector<glm::vec3> row = interpolateVec3(leftColumn[y], rightColumn[y], window.width);
		for (size_t x = 0; x < window.width; x++) {
			uint32_t colour = (255 << 24) + (int(row[x][0]) << 16) + (int(row[x][1]) << 8) + int(row[x][2]);
			window.setPixelColour(x, y, colour);
		}
	}
}

CanvasPoint getRandomPoint(DrawingWindow& window) {
	int x = std::rand() % window.width;
	int y = std::rand() % window.height;
	return CanvasPoint(x, y);
}

CanvasTriangle getRandomTriangle(DrawingWindow& window) {
	return CanvasTriangle(getRandomPoint(window), getRandomPoint(window), getRandomPoint(window));
}

Colour getRandomColour() {
	int r = std::rand() % 256;
	int g = std::rand() % 256;
	int b = std::rand() % 256;
	return Colour(r, g, b);
}

void handleEvent(SDL_Event event, DrawingWindow& window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
		else if (event.key.keysym.sym == SDLK_u) {
			CanvasTriangle tri = getRandomTriangle(window);
			Colour c = getRandomColour();
			drawStrokedTriangle(tri, c, window);
		}
		else if (event.key.keysym.sym == SDLK_f) {
			CanvasTriangle tri = getRandomTriangle(window);
			Colour c = getRandomColour();
			drawFilledTriangle(tri, c, window);
			drawStrokedTriangle(tri, Colour(255, 255, 255), window);
		}
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char* argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	glm::vec3 initialCameraPosition = { 0, 0, 4 };
	float focalLength = 2;

	std::string mtlFilepath = "cornell-box.mtl";
	std::unordered_map<std::string, Colour> palette = readMTL(mtlFilepath);
	std::string objFilepath = "cornell-box.obj";
	std::vector<ModelTriangle> cornellBox = readOBJ(objFilepath, palette, 0.35);
	rasterisedRender(cornellBox, initialCameraPosition, focalLength, window);

	while (true) {
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		window.renderFrame();
	}
}