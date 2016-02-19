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
	virtual void Destroy(Block*) = 0;
	virtual ~BlockFactory() {}
};

// TODO: Specialise this so non-dynamic blocks can be allocated
//	from a pool
template<class T>
struct DefaultBlockFactory : BlockFactory {
	T* Create() override {
		auto bl = new T;
		bl->info = blockInfo;
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

struct VoxelChunk;
struct DynamicBlock;

struct Block {
	BlockInfo* info; // TODO: Could this just be blockID? May be necessary for serialization

	u8 orientation;
	// Tint?

	DynamicBlock* AsDynamic();
	BlockFactory* GetFactory();
};

struct DynamicBlock : Block {
	VoxelChunk* chunk;
	u8 x,y,z;
	
	virtual ~DynamicBlock() {};

	virtual void Update() {}
	virtual void PostRender() {}
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
		bi->factory->blockInfo = bi;

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
		
		// TODO: Use static pool for all deco blocks
		bi->factory = new DefaultBlockFactory<Block>;
		bi->factory->blockInfo = bi;

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
