#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/Light.h"
#include <algorithm>

constexpr auto NUM_PLANE_NORMALS = 39;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

	void ResetCamera();

protected:
	void InitMeshReading();

	void InitBoundingVolume();
	void InitBoundingVolumeDefault(GLuint numVisibleFaces);
	void InitBoundingVolumeMulti(GLuint numWorkGroups, GLuint numFacesPerGroup, GLuint numVisibleFaces);

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
	Shader* testShader;
	Shader* testShader2;
	Shader* testShader3;
	Shader* testShader4;
	Shader* rayTracerShader;
	Shader* finalShader;

	// further testing..
	Light* light;

	// further testing 3..
	GLuint tempSSBO[2];

	GLuint rayTracerNumInvo;
	GLuint rayTracerNumGroups[2]; // for both group x and y

	GLuint image;
};
