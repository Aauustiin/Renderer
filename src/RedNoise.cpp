// C++ Libraries
#include <vector>
#include <unordered_map>

// SDW
#include <DrawingWindow.h>
#include <TextureMap.h>
#include <ModelTriangle.h>

// My Stuff
#include <Rasterising.h>
#include <Objects.h>
#include <Utilities.h>
#include <Parsing.h>
#include <Raytracing.h>
#include <MirrorMaterial.h>

// GLM
#include <glm/glm.hpp>

#define WIDTH 320
#define HEIGHT 240
//#define WIDTH 480
//#define HEIGHT 395
#define IMAGE_PLANE_SCALE 180
#define CAMERA_MOVE_SPEED 0.15
#define CAMERA_ROTATE_SPEED 0.01

// MAIN LOOP

void handleEvent(SDL_Event event, DrawingWindow& window, Camera* cam, RendererState* state) {
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
			case SDLK_a:
				((*cam)).position = (*cam).position + glm::vec3(-CAMERA_MOVE_SPEED, 0, 0);
				break;
			case SDLK_d:
				(*cam).position = (*cam).position + glm::vec3(CAMERA_MOVE_SPEED, 0, 0);
				break;
			case SDLK_w:
				(*cam).position = (*cam).position + glm::vec3(0, 0, -CAMERA_MOVE_SPEED);
				break;
			case SDLK_s:
				(*cam).position = (*cam).position + glm::vec3(0, 0, CAMERA_MOVE_SPEED);
				break;
			case SDLK_SPACE:
				(*cam).position = (*cam).position + glm::vec3(0, CAMERA_MOVE_SPEED, 0);
				break;
			case SDLK_LCTRL:
				(*cam).position = (*cam).position + glm::vec3(0, -CAMERA_MOVE_SPEED, 0);
				break;
			case SDLK_LEFT:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(0, CAMERA_ROTATE_SPEED, 0));
				break;
			case SDLK_RIGHT:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(0, -CAMERA_ROTATE_SPEED, 0));
				break;
			case SDLK_UP:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(CAMERA_ROTATE_SPEED, 0, 0));
				break;
			case SDLK_DOWN:
				(*cam).orientation = rotate((*cam).orientation, glm::vec3(-CAMERA_ROTATE_SPEED, 0, 0));
				break;
			case SDLK_o:
				(*state).orbiting = !(*state).orbiting;
				break;
			case SDLK_1:
				(*state).renderMode = POINTCLOUD;
				break;
			case SDLK_2:
				(*state).renderMode = WIREFRAME;
				break;
			case SDLK_3:
				(*state).renderMode = RASTERISED;
				break;
			case SDLK_4:
				(*state).renderMode = RAYTRACED;
				break;
			case SDLK_5:
				(*state).lightingMode = HARD;
				break;
			case SDLK_6:
				(*state).lightingMode = PROXIMITY;
				break;
			case SDLK_7:
				(*state).lightingMode = SPECULAR;
				break;
			case SDLK_8:
				(*state).lightingMode = AMBIENT;
				break;
			case SDLK_9:
				(*state).lightingMode = GOURAUD;
				break;
			case SDLK_0:
				(*state).lightingMode = PHONG;
				break;
			default:
				break;
		}
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char* argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, IMAGE_PLANE_SCALE, false);
	SDL_Event event;

	RendererState state;
	state.renderMode = WIREFRAME;
	state.orbiting = false;
	state.lightingMode = AMBIENT;

	Camera mainCamera;
	mainCamera.focalLength = 2;
	mainCamera.position = glm::vec3(0, 0, 4);
	mainCamera.orientation = glm::mat3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	glm::vec3 lightCenter = { 0.5, 0.8, 1 };
	float lightRadius = 0.5;
	std::vector<glm::vec3> lights = {lightCenter};
	int numLights = 1;
	for (int i = 0; i < numLights - 1; i++) {
		float v0 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float v1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float v2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		glm::vec3 lightPos = lightCenter + (lightRadius * glm::normalize(glm::vec3(v0, v1, v2)));
		lights.push_back(lightPos);
	}
	

	std::vector<std::string> textureFileNames = {"texture.ppm"};
	std::vector<std::string> materialFileNames = {"textured-cornell-box.mtl"};
	std::vector<std::string> modelFileNames = {"textured-cornell-box.obj", "sphere.obj"};

	std::unordered_map<std::string, TextureMap*> textures = loadTextures(textureFileNames);
	std::unordered_map<std::string, IMaterial*> materials = loadMaterials(materialFileNames, textures);
	std::unordered_map<std::string, std::vector<ModelTriangle>> models = loadModels(modelFileNames,
		materials, {0.35, 0.75});

	glm::vec3 sphereCenter = getCenter(models["sphere.obj"]);
	for (int i = 0; i < models["sphere.obj"].size(); i++) {
		for (int j = 0; j < 3; j++) {
			models["sphere.obj"][i].vertices[j].position.y -= sphereCenter.y;
		}
	}

	std::vector<ModelTriangle> currentModel = models["textured-cornell-box.obj"];
	currentModel[8].material = new MirrorMaterial();
	currentModel[9].material = new MirrorMaterial();
	
	while (true) {
		if (window.pollForInputEvents(event)) handleEvent(event, window, &mainCamera, &state);

		// draw() {
		window.clearPixels();

		switch (state.renderMode) {
			case POINTCLOUD:
				pointcloudRender(currentModel, window, mainCamera);
				break;
			case WIREFRAME:
				wireframeRender(currentModel, window, mainCamera);
				break;
			case RASTERISED:
				rasterisedRender(currentModel, window, mainCamera);
				break;
			case RAYTRACED:
				rayTracedRender(currentModel, lights, window, mainCamera, state.lightingMode);
				break;
		}
		
		window.renderFrame();
		// }

		if (state.orbiting) {
			mainCamera.position = rotateAbout(mainCamera.position, glm::vec3(0, 0, 0), glm::vec3(0, -CAMERA_MOVE_SPEED / 10, 0));
			mainCamera.orientation = lookAt(mainCamera.orientation, mainCamera.position, glm::vec3(0, 0, 0));
		}
	}
	free(currentModel[8].material);
	free(currentModel[9].material);
	for (auto& mat : materials) {
		free(mat.second);
	}
	for (auto& tex : textures) {
		free(tex.second);
	}
}