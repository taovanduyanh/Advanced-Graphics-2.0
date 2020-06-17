#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	camera = new Camera(0.0f, 0.0f, Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 500, RAW_HEIGHT * HEIGHTMAP_Z / 2.0f));
	// for light specular colour
	light = new Light(Vector3(RAW_HEIGHT * HEIGHTMAP_X / 2.0f, 500.0f, RAW_HEIGHT * HEIGHTMAP_Z / 2.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f), (RAW_WIDTH * HEIGHTMAP_X) / 2.0f);
	
	currentShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR"PerPixelFragment.glsl");

	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");
	heightMap->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	if (!currentShader->LinkProgram() || !heightMap->GetTexture()) {
		return;
	}

	SetTextureRepeating(heightMap->GetTexture(), true);


	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	
	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer::~Renderer() {
	delete camera;
	delete light;
	delete heightMap;
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(currentShader->GetProgram());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	UpdateShaderMatrices();
	// for single light
	SetShaderLight(*light);

	heightMap->Draw();

	glUseProgram(0);
	SwapBuffers();
}