#include "SceneNode.h"

SceneNode::SceneNode(Mesh* mesh, Vector4 colour) {
	boundingRadius = 1.0f;
	distanceFromCamera = 0.0f;
	this->mesh = mesh;
	this->colour = colour;
	parent = NULL;
	modelScale = Vector3(1, 1, 1);
}

SceneNode::~SceneNode() {
	for (SceneNode* child : children) {
		delete child;
	}
}

void SceneNode::AddChild(SceneNode* node) {
	if (node == this) {
		return;
	}

	children.push_back(node);
	node->parent = this;
}

void SceneNode::RemoveChild(SceneNode* node) {
	children.erase(remove(children.begin(), children.end(), node), children.end());
}

void SceneNode::Draw(const OGLRenderer& r) {
	if (mesh) {
		mesh->Draw();
	}
}

void SceneNode::Update(float msec) {
	if (parent) {
		worldTransform = parent->worldTransform * transform;
	}
	else {
		worldTransform = transform;
	}

	for (vector<SceneNode*>::const_iterator i = children.begin(); i != children.end(); ++i) {
		(*i)->Update(msec);
	}
}