#include <Colour.h>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>

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