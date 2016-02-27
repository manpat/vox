#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"
#include <array>

enum class GeometryType {
	Cube, Slope, Slab, 
	Cross,
	// VHeight?
};

struct Block;
struct BlockInfo;

struct BlockFactory {
	u16 blockID;
	virtual Block* Create() = 0;
	virtual void Destroy(Block*) = 0;
	virtual ~BlockFactory() {}
};

// TODO: Specialise this so non-dynamic blocks can be allocated
//	from a pool
template<class T>
struct DefaultBlockFactory : BlockFactory {
	T* Create() override {
		auto bl = new T;
		bl->blockID = blockID;
		return bl;
	}

	void Destroy(Block* t) override {
		delete static_cast<T*>(t);
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

struct Chunk;
struct DynamicBlock;

struct Block {
	u16 blockID; // Note that this gets truncated to 14bits on transmission
	u8 orientation; // This only needs two bits
	// Tint?

	DynamicBlock* AsDynamic();
	BlockFactory* GetFactory();
	BlockInfo* GetInfo();
};

struct DynamicBlock : Block {
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

struct BlockRegistry {
	static constexpr u32 MaxBlockInfoCount = 512;

	std::array<BlockInfo, MaxBlockInfoCount> blocks;
	u16 blockInfoCount = 0;

	static BlockRegistry* Get();
	static BlockInfo* AllocateBlockInfo();
	static BlockInfo* GetBlockInfo(u16 blockID);

	static bool IsValidID(u16 blockID);
};

template<class B, template<class> class F = DefaultBlockFactory>
struct BlockRegisterer {
	BlockInfo* bi;

	BlockRegisterer() {
		bi = BlockRegistry::AllocateBlockInfo();

		bi->factory = new F<B>;
		bi->factory->blockID = bi->blockID;

		B::PopulateBlockInfo(bi);

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

struct DecoBlockRegisterer {
	BlockInfo* bi;

	DecoBlockRegisterer(std::string name, GeometryType geometry, 
		std::array<u8, 6> textures, bool doesOcclude) {

		bi = BlockRegistry::AllocateBlockInfo();
		
		// TODO: Use single static pool for all deco blocks
		bi->factory = new DefaultBlockFactory<Block>;
		bi->factory->blockID = bi->blockID;

		bi->name = name;
		bi->geometry = geometry;
		bi->textures = textures;
		bi->doesOcclude = doesOcclude;
		bi->dynamic = false;

		Log("DecoBlockRegisterer") << "Registered new block type: " << bi->name;
	}

	~DecoBlockRegisterer() {
		delete bi->factory;
		bi->factory = nullptr;
	}
};

#endif
