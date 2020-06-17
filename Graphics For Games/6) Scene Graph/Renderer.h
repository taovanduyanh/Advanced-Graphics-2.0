#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/SceneNode.h"
#include "CubeRobot.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer();

	virtual void UpdateScene(float msec);
	virtual void RenderScene();


	// just for testing..
	void RemoveRobot();

protected:
	void DrawNode(SceneNode* n);

	SceneNode* root;
	SceneNode* robot;
	Camera* camera;
};