#ifndef VOXELCHUNK_H
#define VOXELCHUNK_H

#include "common.h"
#include "physics.h"

struct ChunkNeighborhood;
struct ShaderProgram;
struct ChunkManager;
struct Block;

struct VoxelChunk {
	RigidBody* rigidbody;
	Collider* collider;

	Block** blocks;

	u8* geometryData;
	u8* rotationData;
	u8* occlusionData;
	
	u32 vertexBO, faceBO, faceTex;
	u32 width, height, depth;
	u32 numQuads;
	bool voxelsDirty;
	bool blocksDirty;

	mat4 modelMatrix;

	std::weak_ptr<VoxelChunk> self;
	std::weak_ptr<ChunkNeighborhood> neighborhood;
	ivec3 positionInNeighborhood; // Multiple of {width,height,depth} in ?voxelspace?

	VoxelChunk(u32, u32, u32);
	~VoxelChunk();

	void GenerateMesh();
	void UploadMesh();
	void UpdateVoxelData();
	void UpdateBlocks();

	void Update();
	void Render(ShaderProgram*);
	void PostRender();

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