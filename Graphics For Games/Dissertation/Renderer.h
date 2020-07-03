#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"
#include <algorithm>

enum VertexInfo {
	POSITION, COLOUR, TEX_COORD, NORMAL, TANGENT, MAX
};

struct Sphere {
	GLuint numFaces;
	float radius;
	Vector4 center;
	GLint facesID[2];
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
	void InitLastHope();
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
	Shader* lastHope;
	Shader* rayTracerShader;
	Shader* finalShader;

	GLuint image;

	// pre-updated vertices info
	GLuint verticesInfoSSBO[MAX];

	// the selected vertices ID
	GLuint meshesInfoSSBO;
	GLuint selectedFacesIDSSBO;
	GLint* collectedID;

	// For the triangles middle points
	GLuint middlePointsSSBO;
	Vector4* middlePoints;

	// For the spheres..
	GLuint spheresSSBO;
	GLuint numSpheres;
	Sphere* spheres;

	// atomic counter, again...
	GLuint atomicCounter;
};
