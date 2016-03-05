#ifndef CHUNKRENDERER_H
#define CHUNKRENDERER_H

#include "common.h"

struct Chunk;
struct ChunkManager;

struct ChunkRenderInfo {
	u32 vertexBO, faceBO, faceTex;
	u32 numQuads;

	ChunkRenderInfo();
	ChunkRenderInfo(ChunkRenderInfo&&);
	ChunkRenderInfo(const ChunkRenderInfo&) = delete;
	~ChunkRenderInfo();

	// Calls Chunk::BuildMesh()
	void Update(std::shared_ptr<Chunk>);
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