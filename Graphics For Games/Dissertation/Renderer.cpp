#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = new OBJMesh();
	dynamic_cast<OBJMesh*>(triangle)->LoadOBJMesh(MESHDIR"deer.obj");
	//triangle->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();
	camera->SetPosition(Vector3(-15, 785, 2250)); // first view.. (note: for deer mesh)
	//camera->SetPosition(Vector3(2290, 850, 15)); // second view.. (note: for deer mesh)
	//camera->SetYaw(90); // second view.. (note: for deer mesh)

	// shaders..
	meshReader = new Shader(SHADERDIR"MeshReader.glsl");
	volumeCreatorFirst = new Shader(SHADERDIR"VolumeCreatorFirst.glsl");
	volumeCreatorSecond = new Shader(SHADERDIR"VolumeCreatorSecond.glsl");
	volumeCreatorThird = new Shader(SHADERDIR"VolumeCreatorThird.glsl");
	rayTracerShader = new Shader(SHADERDIR"RayTracer.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	testing = new Shader(SHADERDIR"VolumeCreatorTesting.glsl");

	if (!meshReader->LinkProgram() || !volumeCreatorFirst->LinkProgram() || !volumeCreatorSecond->LinkProgram() 
		|| !volumeCreatorThird->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()
		|| !testing->LinkProgram()) {
		return;
	}

	// SSBO to temporarily store the dmin and dmax from the chosen plane normals..
	glGenBuffers(2, tempPlaneDsSSBO);
	
	// first one
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, tempPlaneDsSSBO[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// second one
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO[1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, tempPlaneDsSSBO[1]);
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

	// maybe don't need this anymore..
	glEnable(GL_DEPTH_TEST);

	// alpha blend..
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// fov here..
	fov = 45.0f;

	rayTracerNumInvo = std::round(std::sqrt(static_cast<double>(maxInvosPerGroup)));
	rayTracerNumGroups[0] = std::round(static_cast<double>(width) / rayTracerNumInvo) + 1;
	rayTracerNumGroups[1] = std::round(static_cast<double>(height) / rayTracerNumInvo) + 1;

	// further testing..
	// lighting..
	light = new Light();
	light->SetPosition(Vector3(0, 1000, 50));
	//light->SetColour(Vector4(0.98f, 0.45f, 0.99f, 1.0f));	
	light->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

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

	// further testing 6..
	delete testing;

	currentShader = NULL;

	delete light;

	glDeleteTextures(1, &image);

	glDeleteBuffers(2, tempPlaneDsSSBO);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
	//cout << camera->GetPosition() << endl;
}

void Renderer::ResetCamera() {
	camera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	camera->SetPitch(0.0f);
	camera->SetYaw(0.0f);
}

void Renderer::InitMeshReading() {
	SetCurrentShader(meshReader);
	modelMatrix = Matrix4::Translation(Vector3(0, 0, -10));
	UpdateShaderMatrices();
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPosition"), 1, (float*)&camera->GetPosition());
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numTriangles"), triangle->GetNumFaces());

	// Dividing the work here..
	GLuint numWorkGroups = 1;
	GLuint numInvocations = triangle->GetNumFaces();

	if (numInvocations > maxInvosPerGroup) {
		numWorkGroups = std::round(static_cast<double>(numInvocations / maxInvosPerGroup)) + 1;
		numInvocations = maxInvosPerGroup;
	}

	glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numInvocations, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	triangle->UpdateCollectedID();
}

void Renderer::InitBoundingVolume() {
	GLuint numWorkGroups = 1;
	GLuint numFacesPerGroup = 1;
	GLuint numVisibleFaces = triangle->GetNumVisibleFaces();

	if ((numVisibleFaces * NUM_PLANE_NORMALS) <= maxInvosPerGroup) {
		numFacesPerGroup = numVisibleFaces;
	}
	else {
		numFacesPerGroup = std::round(maxInvosPerGroup / NUM_PLANE_NORMALS);
		double invNumInvoX = 1.0 / numFacesPerGroup;	// maybe cast it?
		numWorkGroups = std::round(static_cast<double>(numVisibleFaces * invNumInvoX)) + 1;
	}

	InitBoundingVolumeMulti(numWorkGroups, numFacesPerGroup, numVisibleFaces);

	//SetCurrentShader(testing);
	//UpdateShaderMatrices();

	//glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), numVisibleFaces);
	//glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numFacesPerGroup, NUM_PLANE_NORMALS, 1);
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::InitBoundingVolumeMulti(GLuint numWorkGroups, GLuint numFacesPerGroup, GLuint numVisibleFaces) {
	// first stage..
	SetCurrentShader(volumeCreatorFirst);
	UpdateShaderMatrices();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVisibleFaces * NUM_PLANE_NORMALS * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BARRIER_BIT, 0);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), numVisibleFaces);

	glDispatchComputeGroupSizeARB(numWorkGroups, 1, 1, numFacesPerGroup, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// second stage..
	SetCurrentShader(volumeCreatorSecond);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tempPlaneDsSSBO[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numWorkGroups * NUM_PLANE_NORMALS * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numFacesPerGroup"), numFacesPerGroup);
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "maxIndex"), numVisibleFaces * NUM_PLANE_NORMALS);

	glDispatchComputeGroupSizeARB(1, 1, 1, numWorkGroups, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// third stage..
	SetCurrentShader(volumeCreatorThird);

	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numGroups"), numWorkGroups);

	glDispatchComputeGroupSizeARB(1, 1, 1, 1, NUM_PLANE_NORMALS, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), triangle->GetTexture());
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fov"), fov);
	glUniform1ui(glGetUniformLocation(currentShader->GetProgram(), "numVisibleFaces"), triangle->GetNumVisibleFaces());

	UpdateShaderMatrices();
	SetShaderLight(*light);

	// HEAVY TESTING......
	//glDispatchComputeGroupSizeARB(width, height, 1, numVisibleFaces, 1, 1);
	//glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
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
}