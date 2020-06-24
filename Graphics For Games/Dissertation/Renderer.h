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
	void InitRayTracing();
	void InitFinalScene();

	Mesh* triangle;
	Mesh* sceneQuad;
	Camera* camera;

	// Max number of work groups, work items and amount of items/invocations allow in a work group
	GLint maxWorkGroups[3];
	GLint maxWorkGroupSizes[3];
	GLint maxWorkItemsPerGroup;

	// testing..
	float fov;
	Shader* testShader;	// remember to change the name later..
	Shader* rayTracerShader;
	Shader* finalShader;

	GLuint image;

	GLuint verticesPosSSBO;
	GLuint verticesNormalSSBO;
	GLuint verticesColoursSSBO;

	GLuint trianglesCount;
	GLuint trianglesAtomic;
};
