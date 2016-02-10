#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "common.h"
#include "stb_voxel_render.h"

struct Camera;
struct VoxelChunk;

struct ChunkNeighborhood {
	std::vector<std::weak_ptr<VoxelChunk>> chunks;
	ivec3 chunkSize;

	void AddChunk(std::shared_ptr<VoxelChunk>);
	void RemoveChunk(std::shared_ptr<VoxelChunk>);
};

struct ChunkManager {
	static constexpr u32 FaceBufferSize = 4<<20; // 4MB
	static constexpr u32 VertexBufferSize = FaceBufferSize*4; // 16MB

	std::vector<std::shared_ptr<VoxelChunk>> chunks;
	std::vector<std::shared_ptr<ChunkNeighborhood>> neighborhoods;
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

	std::shared_ptr<VoxelChunk> CreateChunk(u32 w, u32 h, u32 d, vec3 position = vec3{0}, bool = false);
	std::shared_ptr<ChunkNeighborhood> CreateNeighborhood();

	void Update();
	void Render(Camera*);
};

#endif