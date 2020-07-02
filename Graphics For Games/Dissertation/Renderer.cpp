#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	triangle = Mesh::GenerateCube();
	//triangle->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	//triangle = new OBJMesh();
	//dynamic_cast<OBJMesh*>(triangle)->LoadOBJMesh(MESHDIR"cube.obj");
	sceneQuad = Mesh::GenerateQuad();
	camera = new Camera();

	// for geometry shader SHADERDIR"TrianglesExtraction.glsl"
	meshReader = new Shader(SHADERDIR"AnotherCompute.glsl");
	rayTracerShader = new Shader(SHADERDIR"BasicCompute.glsl");
	finalShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");

	if (!meshReader->LinkProgram() || !rayTracerShader->LinkProgram() || !finalShader->LinkProgram()) {
		return;
	}

	// SSBOs here..
	// Position
	Vector3* positions = triangle->GetPositions();
	Vector4 temp1[8];	// change this later..
	for (int i = 0; i < triangle->GetNumVertices(); ++i) {
		temp1[i] = Vector4(positions[i].x, positions[i].y, positions[i].z, 1.0f);
	}
	glGenBuffers(1, &verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[POSITION]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(Vector4), temp1, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POSITION, verticesInfoSSBO[POSITION]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Colours
	glGenBuffers(1, &verticesInfoSSBO[COLOUR]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[COLOUR]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(Vector4), triangle->GetColours(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COLOUR, verticesInfoSSBO[COLOUR]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Texture coordinates 
	glGenBuffers(1, &verticesInfoSSBO[TEX_COORD]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[TEX_COORD]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumVertices() * sizeof(Vector2), triangle->GetTexCoords(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TEX_COORD, verticesInfoSSBO[TEX_COORD]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Normals
	Vector3* normals = triangle->GetNormals();
	Vector4 temp2[36];	// change this later..
	for (int i = 0; i < 36; ++i) {
		temp2[i] = Vector4(normals[i].x, normals[i].y, normals[i].z, 1.0f);
	}
	glGenBuffers(1, &verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesInfoSSBO[NORMAL]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 36 * sizeof(Vector4), temp2, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NORMAL, verticesInfoSSBO[NORMAL]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Face ID 
	collectedID = new GLint[triangle->GetNumFaces()];
	std::fill_n(collectedID, triangle->GetNumFaces(), -1);
	glGenBuffers(1, &selectedFacesIDSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedFacesIDSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumFaces() * sizeof(GLint), collectedID, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX, selectedFacesIDSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Faces
	glGenBuffers(1, &meshesInfoSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshesInfoSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangle->GetNumFaces() * sizeof(Mesh::Triangle), triangle->GetMeshFaces(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MAX + 2, meshesInfoSSBO);
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

	glEnable(GL_DEPTH_TEST);

	// just testing idea..
	// it might actually work!
	//Vector3 low = modelMatrix * Vector3(-1.0f, -0.003248f, -1.0f);
	//Vector3 high = modelMatrix * Vector3(-1.0f, 1.996752f, 1.0f);
	//Vector3 middlePoint = Vector3((low.x + high.x) / 2, (low.y + high.y) / 2, (low.z + high.z) / 2);
	//Vector3 lowUp = Vector3(low.x, high.y, low.z);
	//Vector3 a = high - middlePoint;
	//Vector3 b = lowUp - middlePoint;
	//Vector3 normal = Vector3::Cross(a, b);
	//normal.Normalise();
	//Vector3 rayOrigin = Vector3(0.0f, 0.0f, 0.0f);
	//Vector3 rayDirection = Vector3(-1.0f, 0.0f, 0.0f);
	//float denom = Vector3::Dot(rayDirection, normal);
	//float inDenom = 1 / denom;
	//Vector3 p = middlePoint - rayOrigin;
	//float t = Vector3::Dot(p, normal) * inDenom;

	// fov here..
	fov = 45.0f;

	init = true;
}

Renderer::~Renderer(void) {
	delete triangle;
	delete sceneQuad;
	delete camera;

	delete meshReader;
	delete rayTracerShader;
	delete finalShader;
	currentShader = NULL;

	delete[] collectedID;

	glDeleteBuffers(MAX, verticesInfoSSBO);
	glDeleteBuffers(1, &selectedFacesIDSSBO);
	glDeleteBuffers(1, &meshesInfoSSBO);
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

void Renderer::ResetIDBuffer() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedFacesIDSSBO);
	// uncomment this to debug..
	//GLint* p = (GLint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	//for (int i = 0; i < triangle->GetNumFaces(); ++i) {
	//	cout << p[i] << endl;
	//}
	//cout << endl;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, collectedID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::InitMeshReading() {
	SetCurrentShader(meshReader);
	modelMatrix = Matrix4::Translation(Vector3(0, 0, -50)) * Matrix4::Scale(Vector3(10, 10, 10));
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraDirection"), 1, (float*)&(camera->GetPosition() - modelMatrix.GetPositionVector()));
	glDispatchComputeGroupSizeARB(1, 1, 1, triangle->GetNumFaces(), 1, 1);
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

	UpdateShaderMatrices();

	glDispatchComputeGroupSizeARB(width, height, 1, 1, 1, 1);
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
	InitRayTracing();
	InitFinalScene();
	SwapBuffers();
	ResetIDBuffer();
}