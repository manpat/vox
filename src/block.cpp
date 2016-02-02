#include "block.h"
#include "blocks/basic.h"

std::array<BlockInfo, 256> BlockRegistry::blocks;
u16 BlockRegistry::blockInfoCount = 0;

BlockRegisterer<SteelBlock> steelBlock;
BlockRegisterer<SteelSlab> steelSlab;
BlockRegisterer<LightThingBlock> lightThingBlock;
BlockRegisterer<RampBlock> rampBlock;
BlockRegisterer<PoleBlock> poleBlock;

BlockRegisterer<ComputerBlock> computerBlock;
BlockRegisterer<TextBlock> textBlock;



mat4 Block::GetOrientationMat() {
	return glm::rotate<f32>(-PI/2.f*orientation, vec3{0,1,0});
}

vec3 Block::GetRelativeCenter() {
	return vec3{x+1.5, z+1.5, -(f32)y-1.5};
}

vec3 Block::GetWorldCenter() {
	vec4 pos = vec4{x+1.5, z+1.5, -(f32)y-1.5, 1};
	return vec3{chunk->modelMatrix * pos};
}
