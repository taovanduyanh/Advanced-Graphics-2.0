#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	Mesh* triangle;
	Camera* camera;

	// testing..
	Shader* sceneShader;
	Shader* computeShader;

	Vector4* colours;
	GLuint posSSBO;
};
