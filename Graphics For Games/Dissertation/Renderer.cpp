#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	mesh = Mesh::GenerateTriangle();
	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();

	sceneShader = new Shader(SHADERDIR"basicVertex.glsl", SHADERDIR"colourFragment.glsl");
	computeShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!sceneShader->LinkProgram() || !computeShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	// new ssbo here..
	glGenBuffers(1, &colourSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colourSSBO);

	colours = new Vector4[3];
	colours[0] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	colours[1] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	colours[2] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(Vector4), colours, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, colourSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// FBO here
	glGenTextures(1, &colourTex);
	glBindTexture(GL_TEXTURE_2D, colourTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &sceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);

	init = true;
}

Renderer::~Renderer(void) {
	delete mesh;
	delete sceneQuad;
	delete camera;
	delete[] colours;

	delete sceneShader;
	delete computeShader;
	delete finalShader;
	currentShader = NULL;

	glDeleteBuffers(1, &colourSSBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

bool test = false;

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colourSSBO);

	// fbo
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// normal scene place
	SetCurrentShader(sceneShader);
	UpdateShaderMatrices();

	mesh->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Compute shader
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

		// testing texture colours
		cout << "Before: " << colours[0].x << " " << colours[0].y << " " << colours[0].z << endl;

		// assume that we are using texture0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colourTex);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
		glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

		glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);	// makes sure the ssbo is written before

		Vector4* p = (Vector4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(colours, p, sizeof(Vector4) * 3);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		cout << "After(colour) : " << colours[0].x << " " << colours[0].y << " " << colours[0].z << endl;
		cout << "After(pixel position): " << colours[1].x << " " << colours[1].y << " " << colours[1].z << endl;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	SetCurrentShader(finalShader);
	// Render the scene quad here..
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	sceneQuad->SetTexutre(colourTex);
	sceneQuad->Draw();

	glUseProgram(0);

	SwapBuffers();
}