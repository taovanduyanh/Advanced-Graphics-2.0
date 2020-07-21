#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	//triangle = Mesh::GenerateQuad();
	triangle = new OBJMesh();
	dynamic_cast<OBJMesh*>(triangle)->LoadOBJMesh(MESHDIR"pyramid.obj");
	//triangle->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();
	camera->SetPosition(Vector3(0, 0, 100));

	// for geometry shader SHADERDIR"TrianglesExtraction.glsl"
	meshReader = new Shader(SHADERDIR"AnotherCompute.glsl");
	testShader = new Shader(SHADERDIR"Testing.glsl");
	rayTracerShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!meshReader->LinkProgram() || !testShader->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

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

	// fov here..
	fov = 45.0f;

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete sceneQuad;
	delete camera;

	delete meshReader;
	delete testShader;
	delete rayTracerShader;
	delete finalShader;
	currentShader = NULL;

	glDeleteTextures(1, &image);
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
	modelMatrix = Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(10, 10, 10));
	UpdateShaderMatrices();
	Vector3 cameraDirection = modelMatrix.GetPositionVector() - camera->GetPosition();
	cameraDirection.Normalise();
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraDirection"), 1, (float*)&cameraDirection);
	glDispatchComputeGroupSizeARB(1, 1, 1, triangle->GetNumFaces(), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	triangle->UpdateCollectedID();
}

void Renderer::InitBoundingBox() {
	// further testing..
	SetCurrentShader(testShader);
	UpdateShaderMatrices();
	glDispatchComputeGroupSizeARB(1, 1, 1, 1, 1, 1);
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
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "scaleVector"), 1, (float*)&modelMatrix.GetScalingVector());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), triangle->GetTexture());
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fov"), fov);

	UpdateShaderMatrices();

	glDispatchComputeGroupSizeARB(1, height, 1, width, 1, 1);
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
	InitBoundingBox();
	InitRayTracing();
	InitFinalScene();
	SwapBuffers();
	ResetBuffers();
}