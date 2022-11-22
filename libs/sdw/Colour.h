#pragma once

#include <iostream>

struct Colour {
	int red{};
	int green{};
	int blue{};
	Colour();
	Colour(int r, int g, int b);
	Colour(uint32_t packedColour);
	uint32_t getPackedColour();
};

std::ostream &operator<<(std::ostream &os, const Colour &colour);
