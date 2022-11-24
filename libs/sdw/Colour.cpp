#include "Colour.h"
#include <utility>

Colour::Colour() = default;
Colour::Colour(int r, int g, int b) : red(r), green(g), blue(b) {}
Colour::Colour(uint32_t packedColour) {
	uint32_t redMask = 0x00FF0000;
	uint32_t greenMask = 0x0000FF00;
	uint32_t blueMask = 0x000000FF;

	int red = (packedColour & redMask) >> 16;
	int green = (packedColour & greenMask) >> 8;
	int blue = (packedColour & blueMask);

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
