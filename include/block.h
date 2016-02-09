#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

enum class GeometryType {
	Cube, Slope, Slab, 
	Cross,
	// VHeight?
};

struct Block;
struct BlockInfo;

struct BlockFactory {
	BlockInfo* blockInfo;
	virtual Block* Create() = 0;
	virtual ~BlockFactory() {}
};

template<class T>
struct DefaultBlockFactory : BlockFactory {
	T* Create() override {
		auto bl = new T;
		bl->info = blockInfo;
		return bl;
	}
};

struct BlockInfo {
	BlockFactory* factory;
	u16 blockID;
	u16 voxelID;

	std::string name;
	GeometryType geometry;
	
	std::array<u8, 6> textures;
	bool dynamic;
	bool doesOcclude;

	// Even with separate IDs, these blocks still need to have their textures
	//	rotated
	bool RequiresIDsForRotations() { return geometry == GeometryType::Slope; }
};

struct VoxelChunk;
struct DynamicBlock;

struct Block {
	BlockInfo* info;
	VoxelChunk* chunk;

	u8 orientation;
	u8 x,y,z;
	// Tint?

	mat4 GetOrientationMat();
	vec3 GetRelativeCenter();
	vec3 GetWorldCenter();

	DynamicBlock* AsDynamic();
};

struct DynamicBlock : Block {
	virtual ~DynamicBlock() {};

	virtual void Update() {}
	virtual void PostRender() {}
	virtual void OnPlace() {}
	virtual void OnBreak() {}
	virtual void OnInteract() {}
};

struct BlockRegistry {
	static std::array<BlockInfo, 256> blocks;
	static u16 blockInfoCount;
};

template<class B, class F = DefaultBlockFactory<B>>
struct BlockRegisterer {
	BlockInfo* bi;

	BlockRegisterer() {
		bi = &BlockRegistry::blocks[BlockRegistry::blockInfoCount];
		bi->blockID = BlockRegistry::blockInfoCount+1;
		bi->factory = new F;
		bi->factory->blockInfo = bi;

		B::PopulateBlockInfo(bi);
		BlockRegistry::blockInfoCount++;

		if(bi->dynamic)
			Log("BlockRegisterer") << "Registered new dynamic block type: " << bi->name;
		else
			Log("BlockRegisterer") << "Registered new block type: " << bi->name;
	}

	~BlockRegisterer() { 
		delete bi->factory;
		bi->factory = nullptr;
	}
};

#endif
