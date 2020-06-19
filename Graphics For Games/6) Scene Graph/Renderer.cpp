#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	CubeRobot::CreateCube();
	camera = new Camera();

	root = new SceneNode();

	//root->AddChild(new CubeRobot());
	root->AddChild(new CubeRobot());

	// remove testing..
	//robot = new CubeRobot();
	//robot->SetTransform(Matrix4::Translation(Vector3(-50, 0, 0)));
	//root->AddChild(robot);

	currentShader = new Shader(SHADERDIR"SceneVertex.glsl", SHADERDIR"SceneFragment.glsl");

	if (!currentShader->LinkProgram()) {
		return;
	}

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	camera->SetPosition(Vector3(0, 30, 175));

	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer::~Renderer() {
	delete camera;
	delete root;
	CubeRobot::DeleteCube();
}

void Renderer::RemoveRobot() {
	root->RemoveChild(robot);
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(msec);
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(currentShader->GetProgram());

	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	DrawNode(root);

	glUseProgram(0);
	SwapBuffers();
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, (float*)&modelMatrix);
		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), (int)n->GetMesh()->GetTexture());
		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}