#pragma once

#include <unordered_map>
#include <Colour.h>
#include <ModelTriangle.h>
#include <IMaterial.h>
#include <TextureMap.h>

std::unordered_map<std::string, IMaterial*> loadMaterials(std::vector<std::string> fileNames,
	std::unordered_map<std::string, TextureMap*> textures);

std::unordered_map<std::string, std::vector<ModelTriangle>> loadModels(std::vector<std::string> fileNames,
	std::unordered_map<std::string, IMaterial*> materials, std::vector<float> scaleFactors);

std::unordered_map<std::string, TextureMap*> loadTextures(std::vector<std::string> textureNames);