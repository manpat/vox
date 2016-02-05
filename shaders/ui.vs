#version 330

uniform mat4 projection;

layout(location=0) in vec3 vertex;
layout(location=1) in vec2 uv;

out float depth;
out vec2 vuv;

void main() {
	vec3 vert = vertex;

	if(vert.z < 0)
		vert.z = -log(-vert.z);
	else
		vert.z = log(max(vert.z, 0) + 1);

	depth = vert.z;
	gl_Position = projection * vec4(vert, 1);
	vuv = uv;
}