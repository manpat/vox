#version 330

uniform mat4 viewProjection;
uniform mat4 model;

layout(location=0) in vec2 vertex;
layout(location=1) in vec2 uv;

out vec2 vuv;

void main() {
	gl_Position = viewProjection * model * vec4(vertex, 0, 1);
	vuv = uv;
}