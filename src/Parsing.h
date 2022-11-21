#pragma once

#include <unordered_map>
#include <Colour.h>
#include <ModelTriangle.h>

std::unordered_map<std::string, Colour> readMTL(std::string& filepath);
std::vector<ModelTriangle> readOBJ(std::string& filepath, std::unordered_map<std::string, Colour> palette, float scaleFactor = 1);