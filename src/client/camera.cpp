#include "camera.h"
#include "shader.h"

std::weak_ptr<Camera> Camera::mainCamera{};

Camera::Camera(f32 a, f32 fs, f32 n, f32 f, ProjectionType p) 
	: aspect{a}, fov{fs}, near{n}, far{f}, projectionType{p} {
	UpdateMatrices();
}

Camera::Camera(const mat4& m) : projectionMatrix{m}, projectionType{Camera::Custom} {
	UpdateMatrices();
}

void Camera::UpdateMatrices() {
	switch(projectionType) {
	case Camera::Perspective:
		projectionMatrix = glm::perspective<f32>(fov, aspect, near, far);
		break;

	case Camera::Orthographic:
		projectionMatrix = glm::ortho<f32>(-aspect*orthoSize, aspect*orthoSize, -orthoSize, orthoSize, near, far);
		break;

	case Camera::Custom:
	default: break;
	}

	viewMatrix = glm::translate(glm::mat4_cast(glm::conjugate(rotation)), -position);

	up = rotation * vec3{0,1,0};
	right = rotation * vec3{1,0,0};
	forward = rotation * vec3{0,0,-1};
}

void Camera::SetUniforms(ShaderProgram* sh) {
	auto vploc = sh->GetUniform("viewProjection");
	if(vploc >= 0){
		glUniformMatrix4fv(vploc, 1, false, glm::value_ptr(projectionMatrix * viewMatrix));
		return;
	}

	auto vloc = sh->GetUniform("view");
	auto ploc = sh->GetUniform("projection");

	glUniformMatrix4fv(vloc, 1, false, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(ploc, 1, false, glm::value_ptr(projectionMatrix));
}
