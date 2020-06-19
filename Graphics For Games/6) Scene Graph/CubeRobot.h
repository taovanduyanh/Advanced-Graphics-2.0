#pragma once
#include "../../nclgl/SceneNode.h"
#include "../../nclgl/OBJMesh.h"

class CubeRobot : public SceneNode {
public:
	CubeRobot();
	~CubeRobot() {};
	virtual void Update(float msec);

	static void CreateCube() {
		cube = new OBJMesh();
		dynamic_cast<OBJMesh*>(cube)->LoadOBJMesh(MESHDIR"Sponze.obj");
		//cube->SetTexutre(SOIL_load_OGL_texture("../../Textures/brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0));

		if (!cube->GetTexture()) {
			return;
		}
	}

	static void DeleteCube() { delete cube; }

protected:
	static Mesh* cube;
	SceneNode* head;
	SceneNode* leftArm;
	SceneNode* rightArm;
};