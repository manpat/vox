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
	u32 chunkIDCounter;

	std::vector<u8> voxelGeometryMap;

	stbvox_mesh_maker mm;
	
	static std::shared_ptr<ChunkManager> Get();

	ChunkManager();
	~ChunkManager();
	void PopulateVoxelInfo();

	// TODO: Reduce interface. The last two arguments aren't particularly useful in most cases
	std::shared_ptr<VoxelChunk> CreateChunk(u32 w, u32 h, u32 d, vec3 position = vec3{0}, bool = false);
	std::shared_ptr<ChunkNeighborhood> CreateNeighborhood();

	void Update();
};

#endif