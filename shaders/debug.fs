#version 330

in vec3 vcolor;
out vec4 outcolor;

void main() {
	outcolor = vec4(vcolor, 1);
}