#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "common.h"

struct ChunkMeshBuilder;
struct Camera;
struct Chunk;

struct ChunkNeighborhood {
	std::vector<std::weak_ptr<Chunk>> chunks;
	ivec3 chunkSize;
	u16 neighborhoodID;

	vec3 position; // Position of chunk at vx{0,0,0}
	quat rotation;

	void UpdateChunkTransforms();
	void UpdateChunkTransform(std::shared_ptr<Chunk>);

	void AddChunk(std::shared_ptr<Chunk>);
	void RemoveChunk(std::shared_ptr<Chunk>);
};

struct ChunkManager {
	std::vector<std::shared_ptr<ChunkNeighborhood>> neighborhoods;
	std::vector<std::shared_ptr<Chunk>> chunks;
	std::shared_ptr<ChunkMeshBuilder> meshBuilder;
	
	static std::shared_ptr<ChunkManager> Get();

	ChunkManager();
	~ChunkManager();

	// NOTE: Client side created chunks WONT be syncronised unless instructed by server
	std::shared_ptr<Chunk> CreateChunk(u32 w, u32 h, u32 d);
	std::shared_ptr<ChunkNeighborhood> CreateNeighborhood();

	std::shared_ptr<Chunk> GetChunk(u16 id);
	void DestroyChunk(u16 id);

	std::shared_ptr<ChunkNeighborhood> GetNeighborhood(u16 id);

	void Update();
};

#endif