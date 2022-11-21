#pragma once

#include <iostream>

struct CanvasPoint {
	int x{};
	int y{};
	float depth{};
	int textureX{};
	int textureY{};

	CanvasPoint();
	CanvasPoint(int xPos, int yPos);
	CanvasPoint(int xPos, int yPos, float pointDepth);
	friend std::ostream &operator<<(std::ostream &os, const CanvasPoint &point);
};
