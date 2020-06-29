#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"
#include <algorithm>

enum VertexInfo {
	POSITION, COLOUR, TEX_COORD, NORMAL, TANGENT, MAX
};

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

	void ResetCamera();

protected:
	void InitMeshReading();
	void FinalizeCollectedID();
	void InitRayTracing();
	void InitFinalScene();

	void ResetAtomicCount();
	void ResetIDLists();
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
	Shader* idFinalizer;
	Shader* rayTracerShader;
	Shader* finalShader;

	GLuint image;

	// pre-updated vertices info
	GLuint verticesInfoSSBO[MAX];

	// the selected vertices ID
	GLuint meshesInfoSSBO;
	GLuint selectedVerticesIDSSBO;
	GLuint finalVerticesIDSSBO;
	GLuint collectedIDCount;
	GLuint idAtomic;
	GLint* collectedID;
	GLint* finalCollectedID;
};
