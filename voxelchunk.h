#ifndef VOXELCHUNK_H
#define VOXELCHUNK_H

#include "common.h"
#include "stb_voxel_render.h"

struct ShaderProgram;

struct VoxelChunk {
	static u32 elementBO;
	static u32 elementBufferSize;

	u8* vertexBuildBuffer;
	u8* faceBuildBuffer;
	u8* blockData;
	stbvox_rgb* colorData;
	
	u32 vertexBO, faceBO, faceTex;
	u32 width, height, depth;
	u32 numQuads;
	bool dirty;

	mat4 modelMatrix;

	stbvox_mesh_maker mm;

	VoxelChunk(u32, u32, u32);
	~VoxelChunk();

	static void LengthenElementBuffer(u32 least);

	void GenerateMesh();
	void Render(ShaderProgram&);

	void SetBlock(u32,u32,u32, u8);
	void SetColor(u32,u32,u32, u8,u8,u8);
	u8 GetBlock(u32,u32,u32);
};

#endif