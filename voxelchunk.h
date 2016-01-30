#ifndef VOXELCHUNK_H
#define VOXELCHUNK_H

#include "common.h"
#include "physics.h"

struct ShaderProgram;
struct ChunkManager;
struct Block;

struct VoxelChunk {
	ChunkManager* manager;
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

	VoxelChunk(ChunkManager*, u32, u32, u32);
	~VoxelChunk();

	void GenerateMesh();
	void UploadMesh();
	void UpdateVoxelData();
	void UpdateBlocks();

	void Update();
	void Render(ShaderProgram*);
	void PostRender();

	Block* CreateBlock(u32,u32,u32, u16);
	void DestroyBlock(u32,u32,u32);
	Block* GetBlock(u32,u32,u32);

	void SetVoxel(u32,u32,u32, u8);
	u8 GetVoxel(u32,u32,u32);
};

#endif