#ifndef CHUNKMESHBUILDER_H
#define CHUNKMESHBUILDER_H

#include "common.h"
#include "stb_voxel_render.h"

struct Chunk;

struct ChunkMeshBuilder {
	static constexpr u32 FaceBufferSize = 4<<20; // 4MB
	static constexpr u32 VertexBufferSize = FaceBufferSize*4; // 16MB

	u8* vertexBuildBuffer;
	u8* faceBuildBuffer;

	std::vector<u8> voxelGeometryMap;

	stbvox_mesh_maker mm;

	ChunkMeshBuilder();
	~ChunkMeshBuilder();
	void PopulateVoxelInfo();

	// Returns number of quads generated
	u32 BuildMesh(std::shared_ptr<Chunk>);
};

#endif