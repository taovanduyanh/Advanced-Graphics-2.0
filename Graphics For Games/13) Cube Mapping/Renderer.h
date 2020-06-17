#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/HeightMap.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/Light.h"

#define COLOUR_DURATION 2.5f

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	virtual ~Renderer();

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	void DrawHeightMap();
	void DrawWater();
	void DrawSkybox();

	void DrawScene();
	void DrawProcess();
	void PresentScene();

	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	Mesh* quad;
	HeightMap* heightMap;
	DirectionalLight* light;
	Camera* camera;

	// post processing
	Shader* sceneShader;
	Shader* processShader;
	GLuint processFBO;
	GLuint bufferFBO;
	GLuint colourTex[2];
	GLuint depthTex;
	Mesh* processQuad;

	float time;
	bool switchColour;

	GLuint cubeMap;

	float waterRotate;
};