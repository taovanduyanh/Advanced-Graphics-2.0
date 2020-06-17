#pragma once
#include "../6) Scene Graph/CubeRobot.h"
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Frustum.h"
#include "../../nclgl/Camera.h"
#include <algorithm>

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer();


	virtual void RenderScene();
	virtual void UpdateScene(float msec);

protected:
	void DrawNode(SceneNode* n);
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();

	SceneNode* root;
	Mesh* quad;
	Camera* camera;

	Frustum frameFrustum;

	vector<SceneNode*> transparentList;
	vector<SceneNode*> opaqueList;
};