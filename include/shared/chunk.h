#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "physics.h"

struct ChunkNeighborhood;
struct ChunkMeshBuilder;
struct ShaderProgram;
struct Block;

// NOTE: I'm not sure about Chunk knowing about physics/numQuads
struct Chunk {
	RigidBody* rigidbody;
	Collider* collider;

	Block* blocks;

	u8* geometryData;
	u8* rotationData;
	u8* occlusionData; // NOTE: Occlusion data not needed on server side
	
	u32 numQuads;
	u16 chunkID;
	u8 width, height, depth;
	bool physicsDirty;
	bool renderDirty;
	bool blocksDirty;

	vec3 position;
	quat rotation;

	std::weak_ptr<Chunk> self;
	std::weak_ptr<ChunkNeighborhood> neighborhood;
	ivec3 positionInNeighborhood; // Multiple of {width,height,depth} in voxelspace

	Chunk(u8, u8, u8);
	~Chunk();

	void GenerateCollider(std::shared_ptr<ChunkMeshBuilder>);
	void UpdateVoxelData();
	void Update();

	// TODO: I'm not sure I like this
	std::shared_ptr<Chunk> GetOrCreateNeighborContaining(ivec3 position);
	void SetNeighborhood(std::shared_ptr<ChunkNeighborhood>);

	// TODO: Methods of creating/destroying/getting blocks in neighboring chunks
	//	would be pretty handy and would simplify calling code.
	Block* CreateBlock(ivec3, u16);
	Block* CreateBlock(ivec3, const std::string&);
	void DestroyBlock(ivec3);
	Block* GetBlock(ivec3);

	ivec3 WorldToVoxelSpace(vec3);
	vec3 VoxelToWorldSpace(ivec3);

	bool InBounds(ivec3);
};

#endif