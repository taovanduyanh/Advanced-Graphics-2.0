#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = Mesh::GenerateQuad();
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
	Vector3* positions = triangle->GetPositions();
	Vector4 temp[4];	// change this later..
	for (int i = 0; i < triangle->GetNumVertices(); ++i) {
		temp[i] = Vector4(positions[i].x, positions[i].y, positions[i].z, 1.0f);
	}
	glGenBuffers(1, &verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[POSITION]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(Vector4), temp, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POSITION, verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Colours
	glGenBuffers(1, &verticesInfoSSBO[COLOUR]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[COLOUR]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(triangle->GetColours()), triangle->GetColours(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COLOUR, verticesInfoSSBO[COLOUR]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Normals
	Vector3* normals = triangle->GetNormals();
	for (int i = 0; i < triangle->GetNumVertices(); ++i) {
		temp[i] = Vector4(normals[i].x, normals[i].y, normals[i].z, 1.0f);
	}
	glGenBuffers(1, &verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[NORMAL]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(Vector4), temp, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NORMAL, verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Vertex ID [triangle->GetNumVertices()]
	collectedID = new GLint[triangle->GetNumVertices()];
	std::fill_n(collectedID, triangle->GetNumVertices(), -1);
	glGenBuffers(1, &selectedVerticesIDSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesIDSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(GLint), collectedID, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX, selectedVerticesIDSSBO);
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

	delete[] collectedID;

	glDeleteBuffers(MAX, verticesInfoSSBO);
	glDeleteBuffers(1, &trianglesAtomic);
	glDeleteTextures(1, &image);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::InitMeshReading() {
	// SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesIDSSBO);

	SetCurrentShader(testShader);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraDirection"), 1, (float*)&camera->GetDirection());
	glDispatchComputeGroupSizeARB(1, 1, 1, triangle->GetNumVertices(), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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

	UpdateShaderMatrices();

	glDispatchComputeGroupSizeARB(width, height, 2, 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);	// makes sure the ssbo is written before

	glBindTexture(GL_TEXTURE_2D, 0);

	GLint* p = (GLint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(p, collectedID, triangle->GetNumVertices() * sizeof(GLint));
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::InitFinalScene() {
	SetCurrentShader(finalShader);
	// Render the scene quad here..
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	sceneQuad->SetTexutre(image);
	sceneQuad->Draw();

	glUseProgram(0);
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	InitMeshReading();
	InitRayTracing();
	InitFinalScene();

	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, trianglesAtomic);

	// reset the triangle count..
	//GLuint* p = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	//*p = 0;
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	//SetCurrentShader(testShader);

	SwapBuffers();
}