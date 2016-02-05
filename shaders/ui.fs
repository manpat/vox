#version 330

// uniform sampler2D tex;

in float depth;
in vec2 vuv;
layout(location=0) out vec4 outcolor;

void main() {
	// vec4 col = texture(tex, vuv);

	outcolor = vec4(vuv, depth, 1);
}