#version 150

flat in uvec4  facedata;
	 in  vec3  voxelspace_pos;
	 in  vec3  vnormal;

uniform vec4 ambient[4];
out vec4 outcolor;

void main() {
	vec3 albedo = facedata.xyz / 255.0;
	vec3 ambient_color = dot(vnormal, ambient[0].xyz) * ambient[1].xyz + ambient[2].xyz;
	
	vec3 lit_color;
	bool emissive = false;
	if (!emissive)
		lit_color = albedo * ambient_color;
	else
		lit_color = albedo;

	vec4 final_color = vec4(lit_color, 1.0);
	outcolor = final_color;
}
