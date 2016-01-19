#include "voxelchunk.h"
#include "shader.h"

u32 VoxelChunk::elementBO = 0;
u32 VoxelChunk::elementBufferSize = 0;
constexpr u32 bufferSize = 2<<20; // 4MB

static Log logger{"VoxelChunk"};

VoxelChunk::VoxelChunk(u32 w, u32 h, u32 d) 
	: width{w}, height{h}, depth{d} {
	// TODO: Use a heuristic here and resize buffers dynamically
	vertexBuildBuffer = new u8[4*bufferSize];
	faceBuildBuffer = new u8[bufferSize];

	blockData = new u8[(width+2)*(height+2)*(depth+2)];
	colorData = new stbvox_rgb[(width+2)*(height+2)*(depth+2)];

	memset(blockData, 0, (width+2)*(height+2)*(depth+2));
	memset(colorData, 255, (width+2)*(height+2)*(depth+2) * sizeof(stbvox_rgb));

	vertexBO = faceBO = faceTex = 0;
	numQuads = 0;
	dirty = true;

	stbvox_init_mesh_maker(&mm);
	auto vinput = stbvox_get_input_description(&mm);
	memset(vinput, 0, sizeof(stbvox_input_description));

	static u8 voxelTypes[] { // TODO: A better way
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_empty, 0, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_solid, 0, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_slab_lower, 0, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_floor_slope_north_is_top, 0, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_floor_slope_north_is_top, 1, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_floor_slope_north_is_top, 2, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_floor_slope_north_is_top, 3, 0),
		STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_crossed_pair, 3, 0),
	};

	vinput->rgb = colorData;
	vinput->blocktype = blockData;
	vinput->block_geometry = voxelTypes;

	stbvox_set_input_stride(&mm, (depth+2)*(height+2), (depth+2));
	stbvox_set_input_range(&mm, 1, 1, 1, width+1, height+1, depth+1);
	stbvox_set_default_mesh(&mm, 0);

	modelMatrix = mat4(1.f);
}

VoxelChunk::~VoxelChunk() {
	delete[] vertexBuildBuffer;
	delete[] faceBuildBuffer;
	delete[] blockData;
	delete[] colorData;
}

void VoxelChunk::LengthenElementBuffer(u32 minNumQuads) {
	if(!elementBO) {
		glGenBuffers(1, &elementBO);
		elementBufferSize = minNumQuads + 100;
	}else{
		while(minNumQuads > elementBufferSize)
			elementBufferSize <<= 1;
	}

	u32* elements = new u32[elementBufferSize*6];
	for(u32 i = 0; i < elementBufferSize; i++) {
		elements[i*6+0] = i*4+0;
		elements[i*6+1] = i*4+2;
		elements[i*6+2] = i*4+1;

		elements[i*6+3] = i*4+0;
		elements[i*6+4] = i*4+3;
		elements[i*6+5] = i*4+2;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementBufferSize*6*sizeof(u32), elements, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	delete[] elements;
}

void VoxelChunk::GenerateMesh() {
	stbvox_reset_buffers(&mm);
	stbvox_set_buffer(&mm, 0, 0, vertexBuildBuffer, bufferSize*4);
	stbvox_set_buffer(&mm, 0, 1, faceBuildBuffer, bufferSize);
	if(!stbvox_make_mesh(&mm)) {
		// TODO: resize and try again/continue
		logger << "Mesh generator ran out of room";
	}

	numQuads = stbvox_get_quad_count(&mm, 0);

	if(!vertexBO) glGenBuffers(1, &vertexBO);
	if(!faceBO) glGenBuffers(1, &faceBO);
	if(!faceTex) glGenTextures(1, &faceTex);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glBufferData(GL_ARRAY_BUFFER, numQuads*4*sizeof(u32), vertexBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_TEXTURE_BUFFER, faceBO);
	glBufferData(GL_TEXTURE_BUFFER, numQuads*sizeof(u32), faceBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	glBindTexture(GL_TEXTURE_BUFFER, faceTex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8UI, faceBO);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void VoxelChunk::Render(ShaderProgram& program) {
	if(dirty) {
		GenerateMesh();
		dirty = false;
	}

	if(numQuads >= elementBufferSize) 
		LengthenElementBuffer(numQuads);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 4, nullptr);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, faceTex);
	glUniform1i(program.GetUniform("facearray"), 0);

	static mat4 coordinateCorrection = glm::rotate<f32>(-PI/2.f, vec3{1,0,0});
	glUniformMatrix4fv(program.GetUniform("model"), 1, false, 
		glm::value_ptr(modelMatrix * coordinateCorrection));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBO);
	glDrawElements(GL_TRIANGLES, numQuads*6, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void VoxelChunk::SetBlock(u32 x, u32 y, u32 z, u8 nval) {
	if(x >= width || y >= height || z >= depth) return;
	auto idx = 1 + z + (y+1)*(depth+2) + (x+1)*(depth+2)*(height+2);
	blockData[idx] = nval;
	dirty = true;
}

void VoxelChunk::SetColor(u32 x, u32 y, u32 z, u8 r, u8 g, u8 b) {
	if(x >= width || y >= height || z >= depth) return;
	auto idx = 1 + z + (y+1)*(depth+2) + (x+1)*(depth+2)*(height+2);
	colorData[idx] = {r,g,b};
	dirty = true;
}

u8 VoxelChunk::GetBlock(u32 x, u32 y, u32 z) {
	if(x >= width || y >= height || z >= depth) return 0;
	auto idx = 1 + z + (y+1)*(depth+2) + (x+1)*(depth+2)*(height+2);
	return blockData[idx];
}