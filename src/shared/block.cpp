#include "block.h"
#include "chunk.h"
#include "blocks/basic.h"

void BlockRegistry::InitBlockInfo() {
	Register("steel", 		GeometryType::Cube, {0,0,0,0,0,0}, true);
	Register("steelslab", 	GeometryType::Slab, {0,0,0,0,0,0}, true);
	Register("lightthing", 	GeometryType::Cube, {1,1,1,1,1,1}, true);
	Register("ramp",		GeometryType::Slope,{0,0,0,0,4,0}, true);
	Register("pole",		GeometryType::Cross,{2,2,2,2,2,2}, false);

	Register<TestDynamicBlock> ("testdynamic", GeometryType::Cube, {0,0,0,3,0,0}, true);
}


void BlockFactory::Create(Block* bl) {
	if(!bl) return;

	bl->dynamic = nullptr;
	bl->blockID = blockID;
	bl->orientation = 0;
}

void BlockFactory::Destroy(Block* bl) {
	if(!bl) return;
	bl->blockID = 0;
}


BlockRegistry* BlockRegistry::Get() {
	static BlockRegistry blockRegistry;
	return &blockRegistry;
}

void BlockRegistry::RegisterWithFactory(std::string name, GeometryType geometry, 
		std::array<u8, 6> textures, bool doesOcclude, BlockFactory* factory) {

	auto blockRegistry = Get();

	auto bi = &blockRegistry->blocks[blockRegistry->blockInfoCount];
	bi->blockID = ++blockRegistry->blockInfoCount; // BlockID zero is invalid

	bi->name = name;
	bi->geometry = geometry;
	bi->textures = textures;
	bi->doesOcclude = doesOcclude;
	bi->factory = factory;
	factory->blockID = bi->blockID;
}

void BlockRegistry::Register(std::string name, GeometryType geometry, 
		std::array<u8, 6> textures, bool doesOcclude) {

	RegisterWithFactory(name, geometry, textures, doesOcclude, new BlockFactory);
}

BlockInfo* BlockRegistry::GetBlockInfo(u16 blockID) {
	if(!IsValidID(blockID)) return nullptr;

	auto blockRegistry = Get();
	return &blockRegistry->blocks[blockID-1];
}

BlockInfo* BlockRegistry::GetBlockInfoByName(const std::string& name) {
	auto blockRegistry = Get();

	auto end = blockRegistry->blocks.begin()+blockRegistry->blockInfoCount;
	auto bi = std::find_if(blockRegistry->blocks.begin(), end, [&name](const BlockInfo& bi) {
		return bi.name == name;
	});

	return (bi == end)?nullptr:&*bi;
}

u16 BlockRegistry::GetBlockIDByName(const std::string& name) {
	auto bi = GetBlockInfoByName(name);
	if(!bi) return 0;
	return bi->blockID;
}


bool BlockRegistry::IsValidID(u16 blockID) {
	// BlockID zero is invalid
	// BlockID greater than number of allocated BlockInfos is invalid

	auto blockRegistry = Get();
	return blockID && (blockID <= blockRegistry->blockInfoCount);
}

/*
	                                                 
	88888888ba  88                        88         
	88      "8b 88                        88         
	88      ,8P 88                        88         
	88aaaaaa8P' 88  ,adPPYba,   ,adPPYba, 88   ,d8   
	88""""""8b, 88 a8"     "8a a8"     "" 88 ,a8"    
	88      `8b 88 8b       d8 8b         8888[      
	88      a8P 88 "8a,   ,a8" "8a,   ,aa 88`"Yba,   
	88888888P"  88  `"YbbdP"'   `"Ybbd8"' 88   `Y8a  
	                                                 
	                                                 
*/
BlockFactory* Block::GetFactory() {
	if(auto info = BlockRegistry::GetBlockInfo(blockID))
		return info->factory;

	return nullptr;
}

BlockInfo* Block::GetInfo() {
	if(auto info = BlockRegistry::GetBlockInfo(blockID))
		return info;

	return nullptr;
}

bool Block::IsValid() {
	// blockID 0 is invalid
	return blockID > 0;
}

// TODO: Nope. Not much need for this
mat4 DynamicBlock::GetOrientationMat() {
	return glm::rotate<f32>(-PI/2.f*block->orientation, vec3{0,1,0});
}

vec3 DynamicBlock::GetRelativeCenter() {
	// TODO: Check
	return chunk->rotation * vec3{x+1.5, z+1.5, -(f32)y-1.5};
}

vec3 DynamicBlock::GetWorldCenter() {
	return chunk->VoxelToWorldSpace(ivec3{x,y,z});
}
