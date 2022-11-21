#include <fstream>
#include <Parsing.h>
#include <Utilities.h>
#include <Objects.h>

std::unordered_map<std::string, Colour> readMTL(std::string& filepath) {
	std::unordered_map<std::string, Colour> result = {};
	std::ifstream inputStream(filepath, std::ifstream::binary);
	std::string nextLine;
	while (!inputStream.eof()) {
		std::getline(inputStream, nextLine); // Name
		auto lineContents = split(nextLine, ' ');
		std::string name = lineContents[1];
		std::getline(inputStream, nextLine); // Value
		lineContents = split(nextLine, ' ');
		int r = std::round(std::stof(lineContents[1]) * 255);
		int g = std::round(std::stof(lineContents[2]) * 255);
		int b = std::round(std::stof(lineContents[3]) * 255);
		Colour colour = Colour(r, g, b);
		result[name] = colour;
		std::getline(inputStream, nextLine); // Empty
	}
	return result;
}

std::vector<ModelTriangle> readOBJ(std::string& filepath, std::unordered_map<std::string, Colour> palette, float scaleFactor) {

	std::ifstream inputStream(filepath, std::ifstream::binary);
	std::string nextLine;

	std::vector<Vertex> vertices = {};
	std::vector<ModelTriangle> result = {};
	std::vector<glm::vec2> texturePoints = {};
	std::vector<std::vector<std::array<int, 2>>> vertexToFace = {};
	std::vector<std::array<int, 3>> faceToVertex = {};
	std::vector<std::string> faceToColour = {};
	std::unordered_map<int, std::array<int, 3>> faceToTexturePoint = {};

	std::string currentColour = "default";
	palette["default"] = Colour(180, 180, 180);

	while (!inputStream.eof()) {

		std::getline(inputStream, nextLine);
		std::vector<std::string> lineContents = split(nextLine, ' ');

		if (lineContents[0] == "v") {
			float x = std::stof(lineContents[1]);
			float y = std::stof(lineContents[2]);
			float z = std::stof(lineContents[3]);
			Vertex newVertex;
			newVertex.position = glm::vec3(x, y, z);
			vertexToFace.push_back({});
			vertices.push_back(newVertex);
		}
		else if (lineContents[0] == "vt") {
			float x = std::stof(lineContents[1]);
			float y = std::stof(lineContents[2]);
			texturePoints.push_back(glm::vec2(x, y));
		}
		else if (lineContents[0] == "f") {
			int currentFaceIndex = faceToVertex.size();
			std::array<int, 3> verticesForFace = {};
			std::array<int, 3> texturePointsForFace = {};
			bool hasTexturePoints = false;

			for (int i = 1; i < lineContents.size(); i++) {
				std::vector<std::string> vertexInfo = split(lineContents[i], '/');
				int vertexIndex = std::stoi(vertexInfo[0]) - 1;
				if (vertexInfo.size() == 3) {
					hasTexturePoints = true;
				    int texturePointIndex = std::stoi(vertexInfo[2]) - 1;
					texturePointsForFace[i] = texturePointIndex;
				}
				verticesForFace[i - 1] = vertexIndex;
				vertexToFace[vertexIndex].push_back({currentFaceIndex, i - 1});
			}
			if (hasTexturePoints) faceToTexturePoint[currentFaceIndex] = texturePointsForFace;
			faceToColour.push_back(currentColour);
			faceToVertex.push_back(verticesForFace);
		}
		else if (lineContents[0] == "usemtl") {
			currentColour = lineContents[1];
		}
	}

	// Scale factor is applied.
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].position *= scaleFactor;
	}

	// Go through the data we've collected and create Model Triangles.
	for (int i = 0; i < faceToVertex.size(); i++) {
		Vertex v0 = vertices[faceToVertex[i][0]];
		Vertex v1 = vertices[faceToVertex[i][1]];
		Vertex v2 = vertices[faceToVertex[i][2]];

		if (faceToTexturePoint.find(i) != faceToTexturePoint.end()) {
			v0.texturePoint = texturePoints[faceToTexturePoint[i][0]];
			v1.texturePoint = texturePoints[faceToTexturePoint[i][1]];
			v2.texturePoint = texturePoints[faceToTexturePoint[i][2]];
		}

		Colour colour = palette[faceToColour[i]];

		glm::vec3 v0toV1 = v1.position - v0.position;
		glm::vec3 v0toV2 = v2.position - v0.position;
		glm::vec3 normal = glm::normalize(glm::cross(v0toV1, v0toV2));

		ModelTriangle triangle = ModelTriangle(v0, v1, v2, colour, normal);
		result.push_back(triangle);
	}

	for (int i = 0; i < vertexToFace.size(); i++) {
		glm::vec3 normal = glm::vec3(0, 0, 0);
		int numFaces = vertexToFace[i].size();
		for (int j = 0; j < numFaces; j++) {
			int faceIndex = vertexToFace[i][j][0];
			normal += result[faceIndex].normal;
		}
		normal /= numFaces;
		for (int j = 0; j < numFaces; j++) {
			int faceIndex = vertexToFace[i][j][0];
			int vertexInFaceIndex = vertexToFace[i][j][1];
			result[faceIndex].vertices[vertexInFaceIndex].normal = normal;
		}
	}

	inputStream.close();
	return result;
}