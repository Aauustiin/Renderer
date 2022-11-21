#pragma once

#include <vector>
#include <DrawingWindow.h>
#include <Objects.h>
#include <ModelTriangle.h>

void pointcloudRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam);
void wireframeRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam);
void rasterisedRender(std::vector<ModelTriangle> model, DrawingWindow& window, Camera cam);