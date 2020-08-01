#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = new OBJMesh();
	dynamic_cast<OBJMesh*>(triangle)->LoadOBJMesh(MESHDIR"deer.obj");
	//triangle->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();
	//camera->SetPosition(Vector3(150, 250, 7500));

	// shaders..
	meshReader = new Shader(SHADERDIR"AnotherCompute.glsl");
	testShader = new Shader(SHADERDIR"Testing.glsl");
	testShader2 = new Shader(SHADERDIR"Testing2.glsl");
	testShader3 = new Shader(SHADERDIR"Testing3.glsl");
	testShader4 = new Shader(SHADERDIR"Testing4.glsl");
	rayTracerShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!meshReader->LinkProgram() || !testShader->LinkProgram() || !testShader2->LinkProgram() || !testShader3->LinkProgram() || !testShader4->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	// further testing 3..
	glGenBuffers(2, tempSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempSSBO[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, tempSSBO[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempSSBO[1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, tempSSBO[1]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
	glBindTexture(GL_TEXTURE_2D, 0);

	// maybe don't need this anymore..
	glEnable(GL_DEPTH_TEST);

	// alpha blend..
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// fov here..
	fov = 45.0f;

	// further testing..
	rayTracerNumInvo = std::round(std::sqrt(static_cast<double>(maxWorkItemsPerGroup)));
	rayTracerNumGroups[0] = std::round(static_cast<double>(width / rayTracerNumInvo)) + 1;
	rayTracerNumGroups[1] = std::round(static_cast<double>(height / rayTracerNumInvo)) + 1;

	// further testing 2..
	light = new Light();
	light->SetPosition(Vector3(20, 100, 0));
	//light->SetColour(Vector4(0.98f, 0.45f, 0.99f, 1.0f));	
	light->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete sceneQuad;
	delete camera;

	delete meshReader;
	delete testShader;
	delete testShader2;
	delete testShader3;
	delete testShader4;
	delete rayTracerShader;
	delete finalShader;
	currentShader = NULL;

	delete light;

	glDeleteTextures(1, &image);

	// further testing..
	glDeleteBuffers(2, tempSSBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::ResetCamera() {
	camera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	camera->SetPitch(0.0f);
	camera->SetYaw(0.0f);
}

void Renderer::ResetBuffers() {
	triangle->ResetSSBOs();
}

void Renderer::InitMeshReading() {
	SetCurrentShader(meshReader);
	modelMatrix = Matrix4::Translation(Vector3(0, 0, -10));
	UpdateShaderMatrices();
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPosition"), 1, (float*)&camera->GetPosition());
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numTriangles"), triangle->GetNumFaces());

	// Dividing the work here..
	GLuint numWorkGroupsX = 1;
	GLuint numInvocations = triangle->GetNumFaces();

	if (numInvocations > maxWorkItemsPerGroup) {
		numWorkGroupsX = std::round(static_cast<double>(numInvocations / maxWorkItemsPerGroup)) + 1;
		numInvocations = maxWorkItemsPerGroup;
	}

	glDispatchComputeGroupSizeARB(numWorkGroupsX, 1, 1, numInvocations, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	triangle->UpdateCollectedID();
}

void Renderer::InitBoundingVolume() {
	GLuint numWorkGroupsX = 1;
	GLuint numInvocationsX = 1;
	GLuint numVisibleFaces = triangle->GetNumVisibleFaces();

	if (numVisibleFaces <= NUM_PLANE_NORMALS) {
		SetCurrentShader(testShader4);

		UpdateShaderMatrices();
		glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), numVisibleFaces);

		glDispatchComputeGroupSizeARB(numWorkGroupsX, 1, 1, numInvocationsX, NUM_PLANE_NORMALS, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	else {
		numInvocationsX = std::round(maxWorkItemsPerGroup / NUM_PLANE_NORMALS);
		double invNumInvoX = 1.0 / numInvocationsX;
		numWorkGroupsX = std::round(static_cast<double>(numVisibleFaces * invNumInvoX)) + 1;

		// further testing 4..
		// first stage..
		SetCurrentShader(testShader);
		UpdateShaderMatrices();

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempSSBO[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numVisibleFaces * NUM_PLANE_NORMALS * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BARRIER_BIT, 0);

		glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), numVisibleFaces);

		glDispatchComputeGroupSizeARB(numWorkGroupsX, 1, 1, numInvocationsX, NUM_PLANE_NORMALS, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// second stage..
		SetCurrentShader(testShader2);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempSSBO[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numWorkGroupsX * NUM_PLANE_NORMALS * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numFacesPerGroup"), numInvocationsX);

		glDispatchComputeGroupSizeARB(1, 1, 1, numWorkGroupsX, NUM_PLANE_NORMALS, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// third stage..
		SetCurrentShader(testShader3);

		glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numGroups"), numWorkGroupsX);

		glDispatchComputeGroupSizeARB(1, 1, 1, 1, NUM_PLANE_NORMALS, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}

void Renderer::InitRayTracing() {
	// Compute shader for ray tracing..
	SetCurrentShader(rayTracerShader);

	// assume that we are using texture0
	glBindTexture(GL_TEXTURE_2D, image);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "image"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, triangle->GetTexture());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuse"), 1);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "scaleVector"), 1, (float*)&modelMatrix.GetScalingVector());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), triangle->GetTexture());
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fov"), fov);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), triangle->GetNumVisibleFaces());

	UpdateShaderMatrices();
	SetShaderLight(*light);

	glDispatchComputeGroupSizeARB(rayTracerNumGroups[0], rayTracerNumGroups[1], 1, rayTracerNumInvo, rayTracerNumInvo, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);	// makes sure the ssbo/image is written before

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::InitFinalScene() {
	SetCurrentShader(finalShader);
	// Render the scene quad here..
	modelMatrix.ToIdentity();
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
	InitBoundingVolume();
	InitRayTracing();
	InitFinalScene();
	SwapBuffers();
	ResetBuffers();
}