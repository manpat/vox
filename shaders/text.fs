#version 330

uniform sampler2D tex;
uniform vec4 color;

in vec2 vuv;
layout(location=0) out vec4 outcolor;

void main() {
	float a = texture(tex, vuv).r;

	float u_buffer = .5f;
	float u_gamma = .5f;
	a = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, a);

	vec3 col = color.rgb;
	col *= a * color.a;

	outcolor = vec4(col, a * color.a);
}