#ifndef CHUNKRENDERER_H
#define CHUNKRENDERER_H

#include "common.h"

struct ChunkManager;
struct VoxelChunk;

struct ChunkRenderInfo {
	u32 vertexBO, faceBO, faceTex;

	ChunkRenderInfo();
	ChunkRenderInfo(ChunkRenderInfo&&);
	ChunkRenderInfo(const ChunkRenderInfo&) = delete;
	~ChunkRenderInfo();

	// Calls VoxelChunk::GenerateMesh()
	void Update(std::shared_ptr<VoxelChunk>);
};

struct ChunkRenderer {
	std::map<u32, ChunkRenderInfo> chunkRenderInfoMap;
	std::vector<u8> voxelTextures;

	u32 textureArray;

	ChunkRenderer();
	~ChunkRenderer();
	void Render();
};

#endif