#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	camera = new Camera(-40, 270, Vector3(-2100, 3300, 2000));
	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");

	currentShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	heightMap->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0));

	if (!currentShader->LinkProgram() || !heightMap->GetTexture()) {
		return;
	}

	SetTextureRepeating(heightMap->GetTexture(), true);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	init = true;
}

Renderer::~Renderer() {
	delete camera;
	delete heightMap;
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(currentShader->GetProgram());
	modelMatrix = Matrix4::Rotation(180, Vector3(1, 0, 0));
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	heightMap->Draw();

	glUseProgram(0);
	SwapBuffers();
}