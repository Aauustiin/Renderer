#include <Rasterising.h>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TextureMap.h>
#include <Utilities.h>

std::vector<CanvasPoint> getLine(CanvasPoint from, CanvasPoint to) {
	std::vector<CanvasPoint> result;
	if ((from.x == to.x) && (from.y == to.y)) {
		result = { from };
	}
	else {
		float xDiff = to.x - from.x;
		float yDiff = to.y - from.y;
		int stepNum = std::max(abs(xDiff), abs(yDiff)) + 1;
		result = interpolate(from, to, stepNum);
	}
	return result;
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 vertexPos, DrawingWindow& window, Camera cam) {
	glm::vec3 cameraSpaceVertex = cam.orientation * (vertexPos - cam.position);
	cameraSpaceVertex.x *= window.scale;
	cameraSpaceVertex.y *= window.scale;

	// Formula taken from the worksheet.
	float u = (window.width / 2) - (cam.focalLength * (cameraSpaceVertex.x / cameraSpaceVertex.z));
	float v = (window.height / 2) + (cam.focalLength * (cameraSpaceVertex.y / cameraSpaceVertex.z));
	return CanvasPoint(u, v, cameraSpaceVertex.z);
}

void drawLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow& window, bool useDepth = true) {
	std::vector<CanvasPoint> line = getLine(from, to);
	uint32_t c = colour.getPackedColour();
	for (int i = 0; i < line.size(); i++) {
		useDepth ? window.setPixelColour(line[i].x, line[i].y, line[i].depth, c) : window.setPixelColour(line[i].x, line[i].y, c);
	}
}

void drawStrokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true) {
	drawLine(triangle.v0(), triangle.v1(), colour, window, useDepth);
	drawLine(triangle.v1(), triangle.v2(), colour, window, useDepth);
	drawLine(triangle.v0(), triangle.v2(), colour, window, useDepth);
}

void drawFlatBottomedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
	float depthV0 = 1 / triangle.v0().depth;
	float depthV1 = 1 / triangle.v1().depth;
	float depthV2 = 1 / triangle.v2().depth;
	// Assume triangle vertices are sorted by y value in ascending order.
	int height = std::abs(triangle.v0().y - triangle.v1().y);
	float xStep1 = (float)(triangle.v1().x - triangle.v0().x) / height;
	float xStep2 = (float)(triangle.v2().x - triangle.v0().x) / height;
	float depthStep1 = (depthV1 - depthV0) / height;
	float depthStep2 = (depthV2 - depthV0) / height;

	// Could use v1 or v2 here for the condition, both should have the same height.
	for (int i = triangle.v0().y; i <= triangle.v1().y; i++) {
		float x1 = (i - triangle.v0().y) * xStep1 + triangle.v0().x;
		float depth1 = (i - triangle.v0().y) * depthStep1 + depthV0;
		float x2 = (i - triangle.v0().y) * xStep2 + triangle.v0().x;
		float depth2 = (i - triangle.v0().y) * depthStep2 + depthV0;
		CanvasPoint p1 = CanvasPoint(std::round(x1), i, depth1);
		CanvasPoint p2 = CanvasPoint(std::round(x2), i, depth2);
		drawLine(p1, p2, colour, window, useDepth);
	}
}

void drawFlatToppedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth) {
	float depthV0 = 1 / triangle.v0().depth;
	float depthV1 = 1 / triangle.v1().depth;
	float depthV2 = 1 / triangle.v2().depth;
	// Assume triangle vertices are sorted by y value in ascending order.
	int height = std::abs(triangle.v2().y - triangle.v0().y);
	float xStep1 = (float)(triangle.v0().x - triangle.v2().x) / height;
	float xStep2 = (float)(triangle.v1().x - triangle.v2().x) / height;
	float depthStep1 = (depthV0 - depthV2) / height;
	float depthStep2 = (depthV1 - depthV2) / height;

	// Could use v0 or v1 here for the condition, both should have the same height.
	for (int i = triangle.v2().y; i >= triangle.v0().y; i--) {
		float x1 = (triangle.v2().y - i) * xStep1 + triangle.v2().x;
		float depth1 = (triangle.v2().y - i) * depthStep1 + depthV2;
		float x2 = (triangle.v2().y - i) * xStep2 + triangle.v2().x;
		float depth2 = (triangle.v2().y - i) * depthStep2 + depthV2;
		CanvasPoint p1 = CanvasPoint(std::round(x1), i, depth1);
		CanvasPoint p2 = CanvasPoint(std::round(x2), i, depth2);
		drawLine(p1, p2, colour, window, useDepth);
	}
}

// Even though we use std::swap, this doesn't seem to sort triangle as a side effect.
void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow& window, bool useDepth = true) {
	// Vertices are sorted in ascending y order.
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());
	if (triangle.v1().y > triangle.v2().y) std::swap(triangle.v1(), triangle.v2());
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());

	if (triangle.v0().y == triangle.v1().y) {
		drawFlatToppedTriangle(triangle, colour, window, useDepth);
	}
	else if (triangle.v1().y == triangle.v2().y) {
		drawFlatBottomedTriangle(triangle, colour, window, useDepth);
	}
	else {
		float midPointX = triangle.v0().x + ((triangle.v1().y - triangle.v0().y) * (triangle.v2().x - triangle.v0().x)) / (triangle.v2().y - triangle.v0().y);
		float midPointDepth = triangle.v0().depth + ((triangle.v1().y - triangle.v0().y) * (triangle.v2().depth - triangle.v0().depth)) / (triangle.v2().y - triangle.v0().y);
		CanvasPoint midPoint = CanvasPoint(std::round(midPointX), triangle.v1().y, midPointDepth);
		drawFlatBottomedTriangle(CanvasTriangle(triangle.v0(), triangle.v1(), midPoint), colour, window, useDepth);
		drawFlatToppedTriangle(CanvasTriangle(triangle.v1(), midPoint, triangle.v2()), colour, window, useDepth);
	}
}

void drawTexturedTriangle(CanvasTriangle triangle, TextureMap texture, DrawingWindow& window) {
	// Vertices are sorted such that v0 has the lowest y value, meaning it's the highest.
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());
	if (triangle.v1().y > triangle.v2().y) std::swap(triangle.v1(), triangle.v2());
	if (triangle.v0().y > triangle.v1().y) std::swap(triangle.v0(), triangle.v1());

	std::vector<CanvasPoint> topToMiddle = getLine(triangle.v0(), triangle.v1());
	std::vector<CanvasPoint> middleToBottom = getLine(triangle.v1(), triangle.v2());
	std::vector<CanvasPoint> topToBottom = getLine(triangle.v0(), triangle.v2());

	CanvasPoint middle;
	for (int i = 0; i < topToBottom.size(); i++) {
		if (topToBottom[i].y == triangle.v1().y) middle = topToBottom[i];
	}

	for (int i = triangle.v0().y; i < middle.y; i++) {

		CanvasPoint point1;
		for (int j = 0; j < topToMiddle.size(); j++) {
			if (topToMiddle[j].y == i) point1 = topToMiddle[j];
		}
		CanvasPoint point2;
		for (int j = 0; j < topToBottom.size(); j++) {
			if (topToBottom[j].y == i) point2 = topToBottom[j];
		}

		std::vector<CanvasPoint> line = getLine(point1, point2);
		for (int j = 0; j < line.size(); j++) {
			int textureIndex = (line[j].texturePoint.y * texture.width) + line[j].texturePoint.x;
			window.setPixelColour(line[j].x, line[j].y, texture.pixels[textureIndex]);
		}
	}
	for (int i = middle.y; i <= triangle.v2().y; i++) {

		CanvasPoint point1;
		for (int j = 0; j < middleToBottom.size(); j++) {
			if (middleToBottom[j].y == i) point1 = middleToBottom[j];
		}
		CanvasPoint point2;
		for (int j = 0; j < topToBottom.size(); j++) {
			if (topToBottom[j].y == i) point2 = topToBottom[j];
		}

		std::vector<CanvasPoint> line = getLine(point1, point2);
		for (int j = 0; j < line.size(); j++) {
			int textureIndex = (line[j].texturePoint.y * texture.width) + line[j].texturePoint.x;
			window.setPixelColour(line[j].x, line[j].y, texture.pixels[textureIndex]);
		}
	}
}

void pointcloudRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		for (int j = 0; j < 3; j++) { // For each vertex in the triangle...
			CanvasPoint point = getCanvasIntersectionPoint(model[i].vertices[j], window, cam); // Get intersection point...
			window.setPixelColour(point.x, point.y, white); // Set colour
		}
	}
}

void wireframeRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(model[i].vertices[0], window, cam);
		CanvasPoint vb = getCanvasIntersectionPoint(model[i].vertices[1], window, cam);
		CanvasPoint vc = getCanvasIntersectionPoint(model[i].vertices[2], window, cam);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawStrokedTriangle(triangle, Colour(255, 255, 255), window);
	}
}

void rasterisedRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam) {
	for (int i = 0; i < model.size(); i++) { // For each triangle in the model...
		CanvasPoint va = getCanvasIntersectionPoint(model[i].vertices[0], window, cam);
		CanvasPoint vb = getCanvasIntersectionPoint(model[i].vertices[1], window, cam);
		CanvasPoint vc = getCanvasIntersectionPoint(model[i].vertices[2], window, cam);
		CanvasTriangle triangle = CanvasTriangle(va, vb, vc);
		drawFilledTriangle(triangle, model[i].colour, window);
	}
}