#version 330

uniform sampler2D tex;

in float depth;
in vec2 vuv;
layout(location=0) out vec4 outcolor;

void main() {
	vec2 uv = vuv;
	uv /= textureSize(tex, 0);

	vec4 col = texture(tex, uv);
	outcolor = col;
}