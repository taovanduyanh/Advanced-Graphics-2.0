#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/Light.h"
#include <algorithm>

constexpr auto NUM_PLANE_NORMALS = 43;

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
	void InitBoundingVolumeMulti(GLuint numWorkGroups, GLuint numFacesPerGroup, GLuint numVisibleFaces);

	void InitRayTracing();
	void InitFinalScene();

	Mesh* triangle;
	Mesh* sceneQuad;
	Camera* camera;

	// Max number of work groups, work items and amount of items/invocations allow in a work group
	GLint maxWorkGroups[3];
	GLint maxWorkGroupSizes[3];
	GLint maxInvosPerGroup;

	// testing..
	float fov;
	Shader* meshReader;	
	Shader* volumeCreatorFirst;
	Shader* volumeCreatorSecond;
	Shader* volumeCreatorThird;
	Shader* rayTracerShader;
	Shader* finalShader;

	// further testing 6..
	// note: this testing process is the same as volumecreator first..
	Shader* testing;

	// further testing..
	Light* light;

	GLuint tempPlaneDsSSBO[2];

	GLuint rayTracerNumInvo;
	GLuint rayTracerNumGroups[2]; // for both group x and y

	GLuint image;
};
