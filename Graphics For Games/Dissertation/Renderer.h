#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/Light.h"
#include <algorithm>

// Note to self: the NUM_PLANE_NORMALS is in common.h file..

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

	void ResetCamera();

protected:
	void CleanImage();

	void InitOperators(Mesh* m);	// I'm not so proud of my naming convention..

	void InitMeshReading(Mesh* m);

	void InitBoundingVolume(Mesh* m);
	void InitBoundingVolumeMulti(GLuint numWorkGroups, GLuint numFacesPerGroup, GLuint numVisibleFaces);

	void InitRayTracing(Mesh* m);
	void InitFinalScene();

	Mesh* triangle;	// not really a triangle, but i'm too lazy to change the name..
	Mesh* sceneQuad;
	Camera* camera;

	// Max number of work groups, work items and amount of items/invocations allow in a work group
	GLint maxWorkGroups[3];
	GLint maxWorkGroupSizes[3];
	GLint maxInvosPerGroup;

	// Shaders..
	Shader* meshReader;	
	Shader* volumeCreatorFirst;
	Shader* volumeCreatorSecond;
	Shader* volumeCreatorThird;
	Shader* rayTracerShader;
	Shader* finalShader;

	// further testing..
	Light* light;

	// Temp d ssbos
	// remove this later?
	GLuint tempPlaneDsSSBO;

	// testing 
	// FBO to clear the image
	GLuint bufferFBO;

	// For ray tracer
	// These will be used to dispatch the ray tracer..
	GLuint rayTracerNumInvo;
	GLuint rayTracerNumGroups[2]; // for both group x and y

	GLuint image;

	float fov;	// important for ray tracing.. (this is needed to calculate the ray's position in world space)
};
