#include <fstream>
#include <Utils.h>
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
	std::vector<std::vector<int>> faces = {}; // The indices of all vertices that are part of each face.
	std::vector<std::string> colours = {}; // The names of colours for each face.
	std::vector<ModelTriangle> result = {};

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
			newVertex.faces = {};
			vertices.push_back(newVertex);
		}
		else if (lineContents[0] == "f") {
			int currentFaceIndex = faces.size();
			std::vector<int> vs = {};

			for (int i = 1; i < lineContents.size(); i++) {
				std::vector<std::string> vertexInfo = split(lineContents[i], '/');
				int vertexIndex = std::stoi(vertexInfo[0]) - 1;
				// if (vertexInfo.size() == 2) {
				//     vertexInfo[1] is a texture point, deal with that
				// }

				vs.push_back(vertexIndex);
				vertices[vertexIndex].faces.push_back({ currentFaceIndex, i - 1 });
			}
			colours.push_back(currentColour);
			faces.push_back(vs);
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
	for (int i = 0; i < faces.size(); i++) {
		ModelTriangle triangle = ModelTriangle(vertices[faces[i][0]].position,
			vertices[faces[i][1]].position,
			vertices[faces[i][2]].position,
			palette[colours[i]]);
		glm::vec3 v0toV1 = triangle.vertices[1] - triangle.vertices[0];
		glm::vec3 v0toV2 = triangle.vertices[2] - triangle.vertices[0];
		glm::vec3 normal = glm::cross(v0toV1, v0toV2);
		triangle.normal = glm::normalize(normal);
		result.push_back(triangle);
	}

	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].normal = glm::vec3(0, 0, 0);
		for (int j = 0; j < vertices[i].faces.size(); j++) {
			vertices[i].normal += result[vertices[i].faces[j][0]].normal;
		}
		vertices[i].normal /= vertices[i].faces.size();
		vertices[i].normal = glm::normalize(vertices[i].normal);
		for (int j = 0; j < vertices[i].faces.size(); j++) {
			result[vertices[i].faces[j][0]].vertexNormals[vertices[i].faces[j][1]] = vertices[i].normal;
		}
	}

	inputStream.close();
	return result;
}