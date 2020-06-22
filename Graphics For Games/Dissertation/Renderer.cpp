#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	mesh = Mesh::GenerateTriangle();
	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();

	sceneShader = new Shader(SHADERDIR"basicVertex.glsl", SHADERDIR"colourFragment.glsl", SHADERDIR"TrianglesExtraction.glsl");
	computeShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!sceneShader->LinkProgram() || !computeShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	// new ssbo here..
	glGenBuffers(1, &colourSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colourSSBO);

	colours = new Vector4[width * height];

	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(Vector4), colours, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, colourSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// testing triangles counting
	// atomic..
	trianglesCount = 0;
	glGenBuffers(1, &trianglesAtomic);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, trianglesAtomic);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &trianglesCount, GL_STATIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, trianglesAtomic);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// FBO here
	// Might need to include the image to perform ray tracing?
	glGenTextures(1, &image);
	glBindTexture(GL_TEXTURE_2D, image);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(1, image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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
	glDeleteBuffers(1, &trianglesAtomic);
	glDeleteTextures(1, &colourTex);
	glDeleteTextures(1, &depthTex);
	glDeleteTextures(1, &image);
	glDeleteFramebuffers(1, &sceneFBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

bool test = false;

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// remove this later..?
	//projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	// SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colourSSBO);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, trianglesAtomic);

	// fbo
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// normal scene place
	SetCurrentShader(sceneShader);
	//UpdateShaderMatrices();

	mesh->Draw();

	// reset the triangle count..
	//GLuint* p = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	//*p = 0;
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

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

		// assume that we are using texture0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "depthTex"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, image);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "image"), 1);
		glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
		glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

		UpdateShaderMatrices();

		glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);	// makes sure the ssbo is written before
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	SetCurrentShader(finalShader);
	// Render the scene quad here..
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	sceneQuad->SetTexutre(image);
	sceneQuad->Draw();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);

	SwapBuffers();
}