#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = Mesh::GenerateTriangle();
	camera = new Camera();

	sceneShader = new Shader(SHADERDIR"basicVertex.glsl", SHADERDIR"colourFragment.glsl");
	computeShader = new Shader(SHADERDIR"BasicCompute.glsl");

	if (!sceneShader->LinkProgram() || !computeShader->LinkProgram()) {
		return;
	}

	// new ssbo here..
	glGenBuffers(1, &posSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);

	colours = new Vector4[3];
	colours[0] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
	colours[1] = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
	colours[2] = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(Vector4), colours, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete camera;
	delete[] colours;

	delete sceneShader;
	delete computeShader;
	currentShader = NULL;

	glDeleteBuffers(1, &posSSBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT);

	// compute shader part
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);

	SetCurrentShader(computeShader);
	glDispatchComputeGroupSizeARB(1, 1, 1, 1024, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);	// external visibility

	// normal scene place
	SetCurrentShader(sceneShader);

	triangle->Draw();


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);

	SwapBuffers();
}