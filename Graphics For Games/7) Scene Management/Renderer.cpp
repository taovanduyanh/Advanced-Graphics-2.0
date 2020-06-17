#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	CubeRobot::CreateCube();
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	camera = new Camera();
	camera->SetPosition(Vector3(0, 100, 750.0f));

	quad = Mesh::GenerateQuad();
	quad->SetTexutre(SOIL_load_OGL_texture(TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0));

	currentShader = new Shader(SHADERDIR"SceneVertex.glsl", SHADERDIR"SceneFragment.glsl");

	if (!currentShader->LinkProgram() || !quad->GetTexture()) {
		return;
	}

	root = new SceneNode();
	for (int i = 0; i < 5; ++i) {
		SceneNode* n = new SceneNode();
		n->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		n->SetTransform(Matrix4::Translation(Vector3(0, 100.0f, -200.0f + 100 * i)));
		n->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		n->SetBoundingRadius(100.0f);
		n->SetMesh(quad);
		root->AddChild(n);
	}

	root->AddChild(new CubeRobot());

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	init = true;
}

Renderer::~Renderer() {
	delete root;
	delete quad;
	delete camera;
	CubeRobot::DeleteCube();
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(msec);
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentList.push_back(from);
		}
		else {
			opaqueList.push_back(from);
		}
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentList.begin(), transparentList.end(), SceneNode::CompareByCameraDistance);
	std::sort(opaqueList.begin(), opaqueList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::ClearNodeLists() {
	opaqueList.clear();
	transparentList.clear();
}

void Renderer::DrawNodes() {
	for (vector<SceneNode*>::const_iterator i = opaqueList.begin(); i != opaqueList.end(); ++i) {
		DrawNode(*i);
	}

	// this one is reverse - end to start
	for (vector<SceneNode*>::const_reverse_iterator i = transparentList.rbegin(); i != transparentList.rend(); ++i) {
		DrawNode(*i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, (float*)&(n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale())));
		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), (int)n->GetMesh()->GetTexture());

		n->Draw(*this);
	}
}

void Renderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(currentShader->GetProgram());
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	DrawNodes();

	glUseProgram(0);
	SwapBuffers();
	ClearNodeLists();
}