#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"
#include <array>

enum class GeometryType {
	Cube, Slope, Slab, 
	Cross,
	// VHeight?
};

struct Chunk;
struct Block;
struct BlockInfo;
struct DynamicBlock;
struct BlockFactory;

struct Block {
	DynamicBlock* dynamic;

	u16 blockID; // Note that this gets truncated to 14bits on transmission
	u8 orientation : 2;
	// Tint?

	DynamicBlock* AsDynamic(); // TODO: This is no longer necessary
	BlockFactory* GetFactory();
	BlockInfo* GetInfo();
	bool InUse();
};

struct DynamicBlock {
	Block* block;
	Chunk* chunk;
	u8 x,y,z;
	
	virtual ~DynamicBlock() {};

	// Shared
	virtual void Update() {}
	virtual void OnMessage() {};

	// Clientside
	virtual void PostRender() {}

	// Serverside
	virtual void OnPlace() {}
	virtual void OnBreak() {}
	virtual void OnInteract() {}

	mat4 GetOrientationMat();
	vec3 GetRelativeCenter();
	vec3 GetWorldCenter();
};

// Block factory initialises a block in place
// Dynamic block factories will initialise
//	and cleanup Block::dynamic
struct BlockFactory {
	u16 blockID;
	virtual void Create(Block*);
	virtual void Destroy(Block*);
	virtual ~BlockFactory() {}
};

template<class T>
struct DefaultDynamicBlockFactory : BlockFactory {
	void Create(Block* bl) override {
		if(!bl) return;

		auto dyn = new T;
		dyn->block = bl;

		bl->blockID = blockID;
		bl->orientation = 0;
		bl->dynamic = dyn;
	}

	void Destroy(Block* bl) override {
		if(!bl) return;
		delete static_cast<T*>(bl->dynamic);
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

struct BlockRegistry {
	static constexpr u32 MaxBlockInfoCount = 512;

	std::array<BlockInfo, MaxBlockInfoCount> blocks;
	u16 blockInfoCount = 0;

	static BlockRegistry* Get();
	static BlockInfo* AllocateBlockInfo();
	static BlockInfo* GetBlockInfo(u16 blockID);

	static bool IsValidID(u16 blockID);
};

struct BlockRegisterer {
	BlockInfo* bi;
	BlockFactory factory;

	BlockRegisterer(std::string name, GeometryType geometry, 
		std::array<u8, 6> textures, bool doesOcclude) {

		bi = BlockRegistry::AllocateBlockInfo();
		
		bi->factory = &factory;
		factory.blockID = bi->blockID;

		bi->name = name;
		bi->geometry = geometry;
		bi->textures = textures;
		bi->doesOcclude = doesOcclude;
		bi->dynamic = false;

		Log("BlockRegisterer") << "Registered new block type: " << bi->name;
	}

	~BlockRegisterer() {
		// We don't want no danglin' pointers
		// That said, bi could already be danglin'
		bi->factory = nullptr;
	}
};

template<class B, template<class> class F = DefaultDynamicBlockFactory>
struct DynamicBlockRegisterer {
	BlockInfo* bi;
	F<B> factory;

	DynamicBlockRegisterer() {
		bi = BlockRegistry::AllocateBlockInfo();

		bi->factory = &factory;
		factory.blockID = bi->blockID;

		B::PopulateBlockInfo(bi);

		if(bi->dynamic)
			Log("DynamicBlockRegisterer") << "Registered new dynamic block type: " << bi->name;
		else
			Log("DynamicBlockRegisterer") << "Registered new block type: " << bi->name;
	}

	~DynamicBlockRegisterer() { 
		// delete bi->factory;
		bi->factory = nullptr;
	}
};

#endif
