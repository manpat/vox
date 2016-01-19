#version 150

in uint attr_vertex;

uniform usamplerBuffer facearray;
// uniform vec3 transform[3];
uniform vec4 camera_pos;
uniform vec3 normal_table[32];
uniform mat4 model;
uniform mat4 view_projection;

flat out uvec4  facedata;
	 // out  vec3  voxelspace_pos;
	 out  vec3  vnormal;
	 // out float  texlerp;
	 out float  amb_occ;

void main() {
	int faceID = gl_VertexID >> 2;
	facedata = texelFetch(facearray, faceID);

	vec3 offset;
	offset.x = float( (attr_vertex       ) & 127u );
	offset.y = float( (attr_vertex >>  7u) & 127u );
	offset.z = float( (attr_vertex >> 14u) & 511u ) * 0.5f;
	amb_occ  = float( (attr_vertex >> 23u) &  63u ) / 63.0;
	// texlerp  = float( (attr_vertex >> 29u)        ) /  7.0;

	vnormal = normal_table[(facedata.w>>2u) & 31u];
	
	// vec3 voxelspace_pos = offset * transform[0];
	// vec3 position  = voxelspace_pos + transform[1];
	// vec3 voxelspace_pos = offset * transform[0];
	// vec3 position  = voxelspace_pos + transform[1];
	gl_Position = view_projection * model * vec4(offset,1.0);
}