#pragma once
#include "Vector3.h"
#include "Vector4.h"

class Light {
public:
	Light() {
		position = Vector3();
		colour = Vector4();
		radius = 0.0f;
	}
	Light(Vector3 position, Vector4 colour, float radius) {
		this->position = position;
		this->colour = colour;
		this->radius = radius;
	};
	~Light() {};

	Vector3 GetPosition() const { return position; }
	void SetPosition(Vector3 p) { position = p; }

	Vector4 GetColour() const { return colour; }
	void SetColour(Vector4 c) { colour = c; }

	float GetRadius() const { return radius; }
	void SetRadius(float r) { radius = r; }

protected:
	float radius;
	Vector3 position;
	Vector4 colour;
};

class DirectionalLight : public Light {
public:
	DirectionalLight() {};
	~DirectionalLight() {};

	Vector3 GetDirection() const { return direction; }
	void SetDirection(Vector3 d) { direction = d; }

protected:
	Vector3 direction;
};