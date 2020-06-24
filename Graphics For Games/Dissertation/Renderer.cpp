#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = Mesh::GenerateTriangle();
	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();
	camera->SetPosition(Vector3(0, 0, 2));

	// for geometry shader SHADERDIR"TrianglesExtraction.glsl"
	testShader = new Shader(SHADERDIR"AnotherCompute.glsl");	// change the name later..
	rayTracerShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!testShader->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	// SSBOs here..
	// Position
	glGenBuffers(1, &verticesPosSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesPosSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(triangle->GetPositions()), triangle->GetPositions(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, verticesPosSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Colours
	glGenBuffers(1, &verticesColoursSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesColoursSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(triangle->GetColours()), triangle->GetColours(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, verticesColoursSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Normals
	glGenBuffers(1, &verticesNormalSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesNormalSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(triangle->GetNormals()), triangle->GetNormals(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, verticesNormalSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// testing triangles counting
	// atomic..
	trianglesCount = 0;
	glGenBuffers(1, &trianglesAtomic);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, trianglesAtomic);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &trianglesCount, GL_STATIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, trianglesAtomic);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// Stuffs related to compute shader..
	// No. of work groups you can create in each dimension
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroups[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroups[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroups[2]);

	// No. of invocations you can have in a work group in each dimensions
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSizes[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSizes[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSizes[2]);

	// Total amount of invocations you can have in a work group
	// => be careful about dividing the work among the invocations
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkItemsPerGroup);

	// Image to shoot rays from..
	glGenTextures(1, &image);
	glBindTexture(GL_TEXTURE_2D, image);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glEnable(GL_DEPTH_TEST);

	// fov here..
	fov = 45.0f;

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete sceneQuad;
	delete camera;

	delete testShader;
	delete rayTracerShader;
	delete finalShader;
	currentShader = NULL;

	glDeleteBuffers(1, &verticesPosSSBO);
	glDeleteBuffers(1, &verticesColoursSSBO);
	glDeleteBuffers(1, &verticesNormalSSBO);
	glDeleteBuffers(1, &trianglesAtomic);
	glDeleteTextures(1, &image);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
	//cout << "Direction: " << camera->GetDirection() << endl;
}

void Renderer::InitRayTracing() {
	// Compute shader for ray tracing..
	SetCurrentShader(rayTracerShader);

	// assume that we are using texture0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "image"), 0);
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fov"), fov);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	UpdateShaderMatrices();

	glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);	// makes sure the ssbo is written before
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::InitFinalScene() {
	SetCurrentShader(finalShader);
	// Render the scene quad here..
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	sceneQuad->SetTexutre(image);
	sceneQuad->Draw();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);
}

void Renderer::RenderScene() {
	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, trianglesAtomic);

	// reset the triangle count..
	//GLuint* p = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	//*p = 0;
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	//SetCurrentShader(testShader);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesPosSSBO);

	//glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraDirection"), 1, (float*)&camera->GetDirection());

	InitRayTracing();
	InitFinalScene();
	SwapBuffers();
}