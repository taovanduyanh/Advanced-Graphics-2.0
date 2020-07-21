#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"
#include <algorithm>

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

	void ResetCamera();

protected:
	void InitMeshReading();
	void InitBoundingBox();
	void InitRayTracing();
	void InitFinalScene();

	void ResetBuffers();

	Mesh* triangle;
	Mesh* sceneQuad;
	Camera* camera;

	// Max number of work groups, work items and amount of items/invocations allow in a work group
	GLint maxWorkGroups[3];
	GLint maxWorkGroupSizes[3];
	GLint maxWorkItemsPerGroup;

	// testing..
	float fov;
	Shader* meshReader;	
	Shader* rayTracerShader;
	Shader* finalShader;

	// further testing..
	Shader* testShader;

	GLuint image;
};
