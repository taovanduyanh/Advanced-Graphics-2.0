#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	Mesh* mesh;
	Mesh* sceneQuad;
	Camera* camera;

	// testing..
	Shader* sceneShader;
	Shader* computeShader;
	Shader* finalShader;

	GLuint sceneFBO;
	GLuint colourTex;
	GLuint depthTex;
	GLuint image;

	Vector4* colours;
	GLuint colourSSBO;

	GLuint trianglesCount;
	GLuint trianglesAtomic;
};
