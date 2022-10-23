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