#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"
#include <algorithm>

enum VertexInfo {
	POSITION, COLOUR, TEX_COORD, NORMAL, TANGENT, INDEX, MAX
};

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	void InitMeshReading();
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
	Shader* meshReader;	// remember to change the name later..
	Shader* rayTracerShader;
	Shader* finalShader;

	GLuint image;

	// pre-updated vertices info
	GLuint verticesInfoSSBO[MAX];

	// the selected vertices ID
	GLuint selectedVerticesIDSSBO;
	GLint* collectedID;

	GLuint trianglesCount;
	GLuint trianglesAtomic;
};
