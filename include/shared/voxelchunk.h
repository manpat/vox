#ifndef VOXELCHUNK_H
#define VOXELCHUNK_H

#include "common.h"
#include "physics.h"

struct ChunkNeighborhood;
struct ShaderProgram;
struct ChunkManager;
struct Block;

// NOTE: I'm not sure about VoxelChunk knowing about physics/numQuads
struct VoxelChunk {
	RigidBody* rigidbody;
	Collider* collider;

	Block** blocks;

	u8* geometryData;
	u8* rotationData;
	u8* occlusionData; // NOTE: Occlusion data not needed on server side
	
	u16 chunkID;
	u32 width, height, depth;
	u32 numQuads;
	bool voxelsDirty;
	bool blocksDirty;

	vec3 position;
	// Orientation
	// mat4 modelMatrix;

	std::weak_ptr<VoxelChunk> self;
	std::weak_ptr<ChunkNeighborhood> neighborhood;
	ivec3 positionInNeighborhood; // Multiple of {width,height,depth} in voxelspace

	VoxelChunk(u32, u32, u32);
	~VoxelChunk();

	// NOTE: GenerateMesh modifies the state of ChunkManager
	//	This is obviously bad design
	void GenerateMesh();
	void UpdateVoxelData();
	void UpdateBlocks();

	void Update();
	void PostRender(); // Only called on client side

	std::shared_ptr<VoxelChunk> GetOrCreateNeighbor(vec3 position);
	void SetNeighborhood(std::shared_ptr<ChunkNeighborhood>);

	Block* CreateBlock(ivec3, u16);
	void DestroyBlock(ivec3);
	Block* GetBlock(ivec3);

	ivec3 WorldToVoxelSpace(vec3);
	vec3 VoxelToWorldSpace(ivec3);

	bool InBounds(ivec3);
};

#endif