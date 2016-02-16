#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "common.h"
#include "stb_voxel_render.h"

struct Camera;
struct VoxelChunk;

struct ChunkNeighborhood {
	std::vector<std::weak_ptr<VoxelChunk>> chunks;
	ivec3 chunkSize;
	// TODO: Neighborhood ID

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

	std::vector<u8> voxelGeometryMap;

	stbvox_mesh_maker mm;
	
	static std::shared_ptr<ChunkManager> Get();

	ChunkManager();
	~ChunkManager();
	void PopulateVoxelInfo();

	// NOTE: Client side created chunks WONT be syncronised unless instructed by server
	std::shared_ptr<VoxelChunk> CreateChunk(u32 w, u32 h, u32 d);
	std::shared_ptr<ChunkNeighborhood> CreateNeighborhood();

	std::shared_ptr<VoxelChunk> GetChunk(u16 id);
	void DestroyChunk(u16 id);

	void Update();
};

#endif