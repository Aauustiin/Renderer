#pragma once

#include <unordered_map>
#include <Colour.h>
#include <ModelTriangle.h>
#include <IMaterial.h>

std::unordered_map<std::string, IMaterial*> readMTL(std::string& filepath);
std::vector<ModelTriangle> readOBJ(std::string& filepath, std::unordered_map<std::string, IMaterial*> materials, float scaleFactor = 1);