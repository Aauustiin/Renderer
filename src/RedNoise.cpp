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

// GLM
#include <glm/glm.hpp>

#define WIDTH 320
#define HEIGHT 240
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
	state.renderMode = RAYTRACED;
	state.orbiting = false;
	state.lightingMode = AMBIENT;

	Camera mainCamera;
	mainCamera.focalLength = 2;
	mainCamera.position = glm::vec3(0, 0, 4);
	mainCamera.orientation = glm::mat3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	glm::vec3 lightPosition = { 0.5, 0.8, 1 };

	std::string mtlFilepath = "cornell-box.mtl";
	std::unordered_map<std::string, IMaterial*> materials = readMTL(mtlFilepath);
	std::string cornellObjFilepath = "cornell-box.obj";
	std::vector<ModelTriangle> cornellBox = readOBJ(cornellObjFilepath, materials, 0.35);
	std::string sphereObjFilepath = "sphere.obj";
	std::vector<ModelTriangle> sphere = readOBJ(sphereObjFilepath, materials, 0.75);
	glm::vec3 sphereCenter = getCenter(sphere);
	for (int i = 0; i < sphere.size(); i++) {
		for (int j = 0; j < 3; j++) {
			sphere[i].vertices[j].position.y -= sphereCenter.y;
		}
	}

	std::vector<ModelTriangle> currentModel = cornellBox;


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
				rayTracedRender(currentModel, lightPosition, window, mainCamera, state.lightingMode);
				break;
		}

		window.renderFrame();
		// }

		if (state.orbiting) {
			mainCamera.position = rotateAbout(mainCamera.position, glm::vec3(0, 0, 0), glm::vec3(0, -CAMERA_MOVE_SPEED / 10, 0));
			mainCamera.orientation = lookAt(mainCamera.orientation, mainCamera.position, glm::vec3(0, 0, 0));
		}
	}
	for (auto& mat : materials) {
		free(mat.second);
	}
}