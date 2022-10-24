#pragma once

#include "TexturePoint.h"
#include <iostream>

struct CanvasPoint {
	int x{};
	int y{};
	float depth{};
	float brightness{};
	TexturePoint texturePoint{};

	CanvasPoint();
	CanvasPoint(int xPos, int yPos);
	CanvasPoint(int xPos, int yPos, float pointDepth);
	CanvasPoint(int xPos, int yPos, float pointDepth, float pointBrightness);
	friend std::ostream &operator<<(std::ostream &os, const CanvasPoint &point);
};
