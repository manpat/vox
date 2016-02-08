#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "common.h"
#include "stb_voxel_render.h"

struct Camera;
struct VoxelChunk;

struct ChunkManager {
	static constexpr u32 FaceBufferSize = 4<<20; // 4MB
	static constexpr u32 VertexBufferSize = FaceBufferSize*4; // 16MB

	std::vector<std::shared_ptr<VoxelChunk>> chunks;
	u8* vertexBuildBuffer;
	u8* faceBuildBuffer;

	std::vector<u8> voxelTypes;
	std::vector<u8> voxelTextures;

	stbvox_mesh_maker mm;
	u32 textureArray;
	
	static std::shared_ptr<ChunkManager> Get();

	ChunkManager();
	~ChunkManager();
	void PopulateVoxelInfo();

	std::shared_ptr<VoxelChunk> CreateChunk(u32 w, u32 h, u32 d, vec3 position, bool = false);

	void Update();
	void Render(Camera*);
};

#endif