#version 150

flat in uvec4  facedata;
	 in  vec3  voxelspace_pos;
	 in  vec3  vnormal;
	 in float  amb_occ;

uniform vec4 ambient[4];
uniform sampler2DArray tex_array;
// uniform samplerBuffer color_table;
uniform vec3 texgen[64];

out vec4 outcolor;

void main() {
	uint tex1_id = facedata.x;
	uint texprojid = facedata.w & 31u;
	// uint color_id  = facedata.z;

	vec3 texgen_s = texgen[texprojid];
	vec3 texgen_t = texgen[texprojid+32u];

	vec3 texturespace_pos = voxelspace_pos;
	vec2 texcoord;
	texcoord.s = dot(texturespace_pos, texgen_s);
	texcoord.t = dot(texturespace_pos, texgen_t);

	vec4 tex1 = texture(tex_array, vec3(texcoord, float(tex1_id)));

	float fragment_alpha = tex1.a;
	vec3 albedo = tex1.xyz;

	if(fragment_alpha < 0.5) discard;
	
	vec3 ambient_color = dot(vnormal, ambient[0].xyz) * ambient[1].xyz + ambient[2].xyz;
	ambient_color = clamp(ambient_color, 0.0, 1.0) * amb_occ;

	vec3 lit_color;
	bool emissive = false;
	if (!emissive)
		lit_color = albedo * ambient_color;
	else
		lit_color = albedo;

	vec4 final_color = vec4(lit_color, 1.0);
	outcolor = final_color;
}