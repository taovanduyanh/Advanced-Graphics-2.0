#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = new OBJMesh();
	dynamic_cast<OBJMesh*>(triangle)->LoadOBJMesh(MESHDIR"Tower.obj");

	//triangle->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	sceneQuad = Mesh::GenerateQuad();

	camera = new Camera();
	//camera->SetPosition(Vector3(-15, 785, 2250)); // first view.. (note: for deer mesh)
	//camera->SetPosition(Vector3(2290, 850, 15)); // second view.. (note: for deer mesh)
	//camera->SetYaw(90); // second view.. (note: for deer mesh)

	// SSBO to temporarily store the dmin and dmax from the chosen plane normals..
	glGenBuffers(1, &tempPlaneDsSSBO);

	// second one, further testing 9..
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, tempPlaneDsSSBO);
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
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvosPerGroup);

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

	sceneQuad->SetTexutre(image);	// set the texture after it is generated..

	// FBO to clear the image..
	glGenFramebuffers(1, &bufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// maybe don't need this anymore..
	glEnable(GL_DEPTH_TEST);

	// alpha blend..
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// fov here..
	fov = 45.0f;

	rayTracerNumInvo = static_cast<GLuint>(std::round(std::sqrt(static_cast<double>(maxInvosPerGroup))));
	rayTracerNumGroups[0] = static_cast<GLuint>(std::round(static_cast<double>(width) / rayTracerNumInvo)) + 1;
	rayTracerNumGroups[1] = static_cast<GLuint>(std::round(static_cast<double>(height) / rayTracerNumInvo)) + 1;

	// further testing..
	// lighting..
	light = new Light();
	light->SetPosition(Vector3(500, 2500, -150));
	//light->SetColour(Vector4(0.98f, 0.45f, 0.99f, 1.0f));	
	light->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	// shaders..
	meshReader = new Shader(SHADERDIR"MeshReader.glsl");
	volumeCreatorFirst = new Shader(SHADERDIR"VolumeCreatorFirst.glsl");
	volumeCreatorSecond = new Shader(SHADERDIR"VolumeCreatorSecond.glsl");
	volumeCreatorThird = new Shader(SHADERDIR"VolumeCreatorThird.glsl");
	rayTracerShader = new Shader(SHADERDIR"RayTracer.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !image 
		||!meshReader->LinkProgram() || !volumeCreatorFirst->LinkProgram() || !volumeCreatorSecond->LinkProgram()
		|| !volumeCreatorThird->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete sceneQuad;
	delete camera;

	delete meshReader;
	delete volumeCreatorFirst;
	delete volumeCreatorSecond;
	delete volumeCreatorThird;
	delete rayTracerShader;
	delete finalShader;
	currentShader = NULL;

	delete light;

	glDeleteTextures(1, &image);

	glDeleteBuffers(1, &tempPlaneDsSSBO);
	glDeleteFramebuffers(1, &bufferFBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene() {
	// Clear the image first..
	// You don't wanna comment this function..
	CleanImage();

	// Draw the first/parent mesh first.. 
	InitOperators(triangle);
	
	std::vector<Mesh*> children = triangle->GetChildren();
	if (!children.empty()) {
		for (Mesh* child : children) {
			InitOperators(child);
		}
	}

	InitFinalScene();
	SwapBuffers();
}

void Renderer::CleanImage() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::InitOperators(Mesh* m) {
	InitMeshReading(m);
	InitBoundingVolume(m);
	InitRayTracing(m);
}

void Renderer::ResetCamera() {
	camera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	camera->SetPitch(0.0f);
	camera->SetYaw(0.0f);
}

void Renderer::InitMeshReading(Mesh* m) {
	// Dividing the work here..
	GLuint numWorkGroups = 1;
	GLuint numInvocations = m->GetNumFaces();

	if (numInvocations > static_cast<GLuint>(maxInvosPerGroup)) {
		numWorkGroups = static_cast<GLuint>(std::round(static_cast<double>(numInvocations / maxInvosPerGroup))) + 1;
		numInvocations = maxInvosPerGroup;
	}

	SetCurrentShader(meshReader);

	// ALWAYS bind the SSBOs of the meshes before doing stuffs..
	m->BindSSBOs();

	modelMatrix = Matrix4::Translation(Vector3(0, -2.5f, -15));
	UpdateShaderMatrices();
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPosition"), 1, (float*)&camera->GetPosition());
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numTriangles"), m->GetNumFaces());

	glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numInvocations, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	m->UpdateCollectedID();
}

void Renderer::InitBoundingVolume(Mesh* m) {
	GLuint numWorkGroups = 1;
	GLuint numFacesPerGroup = 1;
	GLuint numVisibleFaces = m->GetNumVisibleFaces();

	if ((numVisibleFaces * NUM_PLANE_NORMALS) <= static_cast<GLuint>(maxInvosPerGroup)) {
		numFacesPerGroup = numVisibleFaces;
	}
	else {
		numFacesPerGroup = static_cast<GLuint>(std::round(maxInvosPerGroup / NUM_PLANE_NORMALS));
		double invNumInvoX = 1.0 / numFacesPerGroup;	// maybe cast it?
		numWorkGroups = static_cast<GLuint>(std::round(static_cast<double>(numVisibleFaces * invNumInvoX))) + 1;
	}

	// ALWAYS bind the SSBOs of the meshes before doing stuffs..
	m->BindSSBOs();

	InitBoundingVolumeMulti(numWorkGroups, numFacesPerGroup, numVisibleFaces);
}

void Renderer::InitBoundingVolumeMulti(GLuint numWorkGroups, GLuint numFacesPerGroup, GLuint numVisibleFaces) {
	// first stage..
	SetCurrentShader(volumeCreatorFirst);
	UpdateShaderMatrices();

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), numVisibleFaces);

	glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numFacesPerGroup, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// second stage..
	SetCurrentShader(volumeCreatorSecond);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numWorkGroups * NUM_PLANE_NORMALS * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numFacesPerGroup"), numFacesPerGroup);
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "maxIndex"), numVisibleFaces * NUM_PLANE_NORMALS);

	// Sometimes num of work groups multiply with the number of plane normals is greater than the maximum invocations per group..
	bool isOverMaxInvosPerGroup = (numWorkGroups * NUM_PLANE_NORMALS) > static_cast<GLuint>(maxInvosPerGroup);

	isOverMaxInvosPerGroup ? glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numFacesPerGroup, NUM_PLANE_NORMALS, 1) 
							: glDispatchComputeGroupSizeARB(1, 1, 1, numWorkGroups, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// third stage..
	SetCurrentShader(volumeCreatorThird);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numGroups"), numWorkGroups);

	glDispatchComputeGroupSizeARB(1, 1, 1, 1, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::InitRayTracing(Mesh* m) {
	// Compute shader for ray tracing..
	SetCurrentShader(rayTracerShader);

	// ALWAYS bind the SSBOs of the meshes first..
	m->BindSSBOs();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m->GetTexture());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuse"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), m->GetTexture());
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fov"), fov);
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), m->GetNumVisibleFaces());

	UpdateShaderMatrices();
	SetShaderLight(*light);

	// HEAVY TESTING......
	//glDispatchComputeGroupSizeARB(width, height, 1, numVisibleFaces, 1, 1);
	//glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
	glDispatchComputeGroupSizeARB(rayTracerNumGroups[0], rayTracerNumGroups[1], 1, rayTracerNumInvo, rayTracerNumInvo, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);	// makes sure the image is written before

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::InitFinalScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SetCurrentShader(finalShader);
	// Render the scene quad here..
	modelMatrix.ToIdentity();
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	sceneQuad->Draw();

	glUseProgram(0);
}