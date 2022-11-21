#include "CanvasPoint.h"

CanvasPoint::CanvasPoint() :
	textureX(-1),
	textureY(1) {}

CanvasPoint::CanvasPoint(int xPos, int yPos) :
	x(xPos),
	y(yPos),
	depth(0.0),
	textureX(-1),
	textureY(1) {}

CanvasPoint::CanvasPoint(int xPos, int yPos, float pointDepth) :
	x(xPos),
	y(yPos),
	depth(pointDepth),
	textureX(-1),
	textureY(1) {}

std::ostream &operator<<(std::ostream &os, const CanvasPoint &point) {
	os << "(" << point.x << ", " << point.y << ", " << point.depth << ")";
	return os;
}
