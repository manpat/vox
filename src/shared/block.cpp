#include "block.h"
#include "blocks/basic.h"

auto decoBlocks = {
	BlockRegisterer {"steel", 		GeometryType::Cube, {0,0,0,0,0,0}, true},
	BlockRegisterer {"steelslab", 	GeometryType::Slab, {0,0,0,0,0,0}, true},

	BlockRegisterer {"lightthing", 	GeometryType::Cube, {1,1,1,1,1,1}, true},
	BlockRegisterer {"ramp",		GeometryType::Slope,{0,0,0,0,4,0}, true},
	BlockRegisterer {"pole",		GeometryType::Cross,{2,2,2,2,2,2}, false},
};

// DynamicBlockRegisterer<ComputerBlock> computerBlock;
// DynamicBlockRegisterer<TextBlock> textBlock;
DynamicBlockRegisterer<TestDynamicBlock> testDynamicBlock;


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

BlockInfo* BlockRegistry::AllocateBlockInfo() {
	auto blockRegistry = Get();

	auto bi = &blockRegistry->blocks[blockRegistry->blockInfoCount];
	bi->blockID = ++blockRegistry->blockInfoCount; // BlockID zero is invalid
	return bi;
}

BlockInfo* BlockRegistry::GetBlockInfo(u16 blockID) {
	if(!IsValidID(blockID)) return nullptr;

	auto blockRegistry = Get();
	return &blockRegistry->blocks[blockID-1];
}

bool BlockRegistry::IsValidID(u16 blockID) {
	// BlockID zero is invalid
	// BlockID greater than number of allocated BlockInfos is invalid

	auto blockRegistry = Get();
	return blockID && (blockID <= blockRegistry->blockInfoCount);
}


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
