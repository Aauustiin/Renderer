#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <Colour.h>
#include <glm/vec2.hpp>

class TextureMap {
public:
	size_t width;
	size_t height;
	std::vector<uint32_t> pixels;

	TextureMap();
	TextureMap(const std::string &filename);
	Colour GetValue(glm::vec2 texturePoint);
	friend std::ostream &operator<<(std::ostream &os, const TextureMap &point);
};
