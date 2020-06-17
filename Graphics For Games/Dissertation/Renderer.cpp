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

	// for the maximum values..
	{
		// No. of work groups you can create in each dimension
		int numWorkGroups[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &numWorkGroups[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &numWorkGroups[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &numWorkGroups[2]);

		// No. of invocations you can have in a work group in each dimensions
		int workGroupSize[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSize[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSize[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSize[2]);

		// Total amount of invocations you can have in a work group
		// => this means you have to be careful about dividing the work
		int numInvocations;
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &numInvocations);
		cout << numInvocations << endl;

		glDispatchComputeGroupSizeARB(1, 1, 1, workGroupSize[0], 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);	// external visibility
	}

	// normal scene place
	SetCurrentShader(sceneShader);

	triangle->Draw();


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);

	SwapBuffers();
}