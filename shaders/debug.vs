#version 330

uniform mat4 projection;
uniform mat4 view;

layout(location=0) in vec3 vertex;
layout(location=8) in vec3 color;

out vec3 vcolor;

void main() {
	vec4 pos = view * vec4(vertex, 1);
	gl_Position = projection * pos;
	vcolor = color;

	gl_PointSize = max(10.f + pos.z/10.f, 10.f);
}