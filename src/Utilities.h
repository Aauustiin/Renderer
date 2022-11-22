#pragma once

#include <vector>
#include <CanvasPoint.h>
#include <ModelTriangle.h>
#include <glm/glm.hpp>

std::vector<float> interpolate(float from, float to, int numberOfValues);

std::vector<CanvasPoint> interpolate(CanvasPoint from, CanvasPoint to, int numberOfValues);

std::vector<glm::vec3> interpolate(glm::vec3 from, glm::vec3 to, int numberOfValues);

glm::vec3 rotate(glm::vec3 subject, glm::vec3 rotation);

glm::mat3 rotate(glm::mat3 subject, glm::vec3 rotation);

glm::vec3 rotateAbout(glm::vec3 subject, glm::vec3 origin, glm::vec3 rotation);

glm::mat3 lookAt(glm::mat3 subjectOientation, glm::vec3 subjectPosition, glm::vec3 target);

glm::vec3 getCenter(std::vector<ModelTriangle> model);

void printVec3(glm::vec3 x);

std::vector<std::string> split(const std::string& line, char delimiter);

float triangleArea(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);

float triangleInterpolation(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
	float a0, float a1, float a2,
	glm::vec3 p);

glm::vec2 triangleInterpolation(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
	glm::vec2 a0, glm::vec2 a1, glm::vec2 a2,
	glm::vec3 p);