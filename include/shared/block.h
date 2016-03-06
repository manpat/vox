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

	// There's a whole 5.75 bytes free here in 64bit builds

	BlockFactory* GetFactory();
	BlockInfo* GetInfo();
	bool IsValid();
};

struct DynamicBlock {
	Block* block;
	Chunk* chunk;
	u8 x,y,z;
	
	virtual ~DynamicBlock() {};

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
struct DynamicBlockFactory : BlockFactory {
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
		bl->dynamic = nullptr;
		bl->blockID = 0;
	}
};

struct BlockInfo {
	BlockFactory* factory;
	u16 blockID;
	u16 voxelID;

	std::string name;
	GeometryType geometry;
	
	std::array<u8, 6> textures;
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
	static void RegisterWithFactory(std::string, GeometryType, std::array<u8, 6>, bool, BlockFactory*);

	// Use these to register new block types, preferably in InitBlockInfo
	template<class T>
	static void Register(std::string, GeometryType, std::array<u8, 6>, bool);
	static void Register(std::string, GeometryType, std::array<u8, 6>, bool);
	
	static void InitBlockInfo();
	static BlockInfo* GetBlockInfo(u16 blockID);
	static BlockInfo* GetBlockInfoByName(const std::string& name);
	static u16 GetBlockIDByName(const std::string& name);

	static bool IsValidID(u16 blockID);
};

template<class T>
void BlockRegistry::Register(std::string n, GeometryType g, std::array<u8, 6> t, bool o) {
	RegisterWithFactory(n,g,t,o, new DynamicBlockFactory<T>);
}

#endif
