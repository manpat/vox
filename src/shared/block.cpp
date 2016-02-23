#include "block.h"
#include "blocks/basic.h"

auto decoBlocks = {
	DecoBlockRegisterer {"steel", 		GeometryType::Cube, {0,0,0,0,0,0}, true},
	DecoBlockRegisterer {"steelslab", 	GeometryType::Slab, {0,0,0,0,0,0}, true},

	DecoBlockRegisterer {"lightthing", 	GeometryType::Cube, {1,1,1,1,1,1}, true},
	DecoBlockRegisterer {"ramp",		GeometryType::Slope,{0,0,0,0,4,0}, true},
	DecoBlockRegisterer {"pole",		GeometryType::Cross,{2,2,2,2,2,2}, false},
};

// BlockRegisterer<ComputerBlock> computerBlock;
// BlockRegisterer<TextBlock> textBlock;
BlockRegisterer<TestDynamicBlock> testDynamicBlock;

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


DynamicBlock* Block::AsDynamic() {
	if(!info || !info->dynamic) return nullptr;
	return static_cast<DynamicBlock*>(this);
}

BlockFactory* Block::GetFactory() {
	if(!info) return nullptr;
	return info->factory;
}

// TODO: Nope. Not much need for this
mat4 DynamicBlock::GetOrientationMat() {
	return glm::rotate<f32>(-PI/2.f*orientation, vec3{0,1,0});
}

vec3 DynamicBlock::GetRelativeCenter() {
	// TODO: Check
	return chunk->rotation * vec3{x+1.5, z+1.5, -(f32)y-1.5};
}

vec3 DynamicBlock::GetWorldCenter() {
	return chunk->VoxelToWorldSpace(ivec3{x,y,z});
}
