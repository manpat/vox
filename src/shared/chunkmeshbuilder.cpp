#include "chunkmeshbuilder.h"
#include "block.h"
#include "chunk.h"

static Log logger{"ChunkMeshBuilder"};

ChunkMeshBuilder::ChunkMeshBuilder() {
	vertexBuildBuffer = new u8[VertexBufferSize];
	faceBuildBuffer = new u8[FaceBufferSize];

	PopulateVoxelInfo();

	stbvox_init_mesh_maker(&mm);
	auto vinput = stbvox_get_input_description(&mm);
	memset(vinput, 0, sizeof(stbvox_input_description));
	vinput->block_geometry = &voxelGeometryMap[0];
}

ChunkMeshBuilder::~ChunkMeshBuilder() {
	delete[] vertexBuildBuffer;
	delete[] faceBuildBuffer;
}

void ChunkMeshBuilder::PopulateVoxelInfo() {
	static std::map<GeometryType, u32> voxelGeomMap {
		{GeometryType::Cube,	STBVOX_GEOM_solid},
		{GeometryType::Slab,	STBVOX_GEOM_slab_lower},
		{GeometryType::Cross,	STBVOX_GEOM_crossed_pair},
		{GeometryType::Slope,	STBVOX_GEOM_floor_slope_north_is_top},
	};

	voxelGeometryMap = {STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_empty, 0, 0)};
	u16 id = 1;

	auto blockRegistry = BlockRegistry::Get();

	for(u16 i = 0; i < blockRegistry->blockInfoCount; i++) {
		auto& bt = blockRegistry->blocks[i];
		bt.voxelID = id++;

		voxelGeometryMap.push_back(STBVOX_MAKE_GEOMETRY(voxelGeomMap[bt.geometry], 0, 0));
		if(bt.RequiresIDsForRotations()) {
			id += 3;
			for(u8 r = 1; r < 4; r++)
				voxelGeometryMap.push_back(STBVOX_MAKE_GEOMETRY(voxelGeomMap[bt.geometry], r, 0));
		}

		logger << "Voxel type created for " << bt.name << ": " << bt.blockID << " -> " << bt.voxelID;
	}
}

u32 ChunkMeshBuilder::BuildMesh(std::shared_ptr<Chunk> ch) {
	auto vinput = stbvox_get_input_description(&mm);
	vinput->blocktype = ch->geometryData;
	vinput->lighting = ch->occlusionData; // NOTE: This can/should be omitted on the serverside
	vinput->rotate = ch->rotationData;

	u32 w = ch->width;
	u32 h = ch->height;
	u32 d = ch->depth;

	stbvox_set_input_stride(&mm, (d+2)*(h+2), (d+2));
	stbvox_set_input_range(&mm, 1, 1, 1, w+1, h+1, d+1);
	stbvox_set_default_mesh(&mm, 0);

	stbvox_reset_buffers(&mm);
	stbvox_set_buffer(&mm, 0, 0, vertexBuildBuffer, ChunkMeshBuilder::VertexBufferSize);
	stbvox_set_buffer(&mm, 0, 1, faceBuildBuffer, ChunkMeshBuilder::FaceBufferSize);

	if(!stbvox_make_mesh(&mm)) {
		// TODO: resize and try again/continue
		// Low priority, chunks should be relatively small anyway
		logger << "Mesh generator ran out of room";
	}

	return stbvox_get_quad_count(&mm, 0);
}
