#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include "SDL.h"

class DrawingWindow {

public:
	size_t width;
	size_t height;
	float scale;

private:
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	std::vector<uint32_t> pixelBuffer;

	// The values we store here are actually the inverses of the depths.
	std::vector<float> depthBuffer;

public:
	DrawingWindow();
	DrawingWindow(int w, int h, float s, bool fullscreen);
	void renderFrame();
	void savePPM(const std::string &filename) const;
	void saveBMP(const std::string &filename) const;
	bool pollForInputEvents(SDL_Event &event);
	void setPixelColour(size_t x, size_t y, uint32_t colour);
	void setPixelColour(size_t x, size_t y, float depth, uint32_t colour);
	uint32_t getPixelColour(size_t x, size_t y);
	void clearPixels();
};

void printMessageAndQuit(const std::string &message, const char *error);
