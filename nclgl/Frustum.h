#pragma once
#include "Plane.h"
#include "Matrix4.h"
#include "SceneNode.h"

class Matrix4;	// compile the matrix4 class first

class Frustum {
public:
	Frustum() {};
	~Frustum() {};

	void FromMatrix(const Matrix4& mat);
	bool InsideFrustum(SceneNode& n);

protected:
	Plane planes[6];
};