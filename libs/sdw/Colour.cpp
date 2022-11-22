#include "Colour.h"
#include <utility>

Colour::Colour() = default;
Colour::Colour(int r, int g, int b) : red(r), green(g), blue(b) {}
Colour::Colour(uint32_t packedColour) {
	uint32_t redMask = 0xFF000000;
	uint32_t greenMask = 0xFF0000;
	uint32_t blueMask = 0xFF00;

	int red = (packedColour & redMask) >> 24;
	int green = (packedColour & greenMask) >> 16;
	int blue = (packedColour & blueMask) >> 8;

	(*this).red = red;
	(*this).green = green;
	(*this).blue = blue;
}
uint32_t Colour::getPackedColour() {
	uint32_t packedColour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
	return packedColour;
}

std::ostream &operator<<(std::ostream &os, const Colour &colour) {
	os << " [" << colour.red << ", "
	   << colour.green << ", "
	   << colour.blue << "]";
	return os;
}
