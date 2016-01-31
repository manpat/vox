#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

struct ShaderProgram;

struct Camera {
	static std::weak_ptr<Camera> mainCamera;
	static std::weak_ptr<Camera> uiCamera;

	// Set in UpdateMatrices
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec3 forward;
	vec3 right;
	vec3 up;

	// Updated by user
	vec3 position = vec3{0};
	quat rotation = quat{1,0,0,0};

	f32 aspect;
	union { f32 fov; f32 orthoSize; };
	f32 near, far;
	bool isPerspective;

	Camera(f32 a = 4.f/3.f, f32 fs = PI/3.f, f32 n = 0.01f, f32 f = 1000.f, bool p = true);

	void UpdateMatrices();
	void SetUniforms(ShaderProgram*);
};

#endif