#include "chunkmanager.h"
#include "voxelchunk.h"
#include "physics.h"
#include "block.h"

#include "stb_voxel_render.h"

static Log logger{"VoxelChunk"};

VoxelChunk::VoxelChunk(u32 w, u32 h, u32 d) 
	: width{w}, height{h}, depth{d} {

	chunkID = 0;

	u64 size = (width+2)*(height+2)*(depth+2);
	geometryData = new u8[size];
	rotationData = new u8[size];
	occlusionData = new u8[size];

	blocks = new Block*[width*height*depth];
	
	memset(geometryData, 0, size);
	memset(rotationData, 0, size);
	memset(occlusionData, 255, size);

	memset(blocks, 0, width*height*depth * sizeof(Block*));

	numQuads = 0;
	blocksDirty = false;
	voxelsDirty = true;

	position = vec3{0.f};
	// modelMatrix = mat4(1.f);

	btScalar mass = 0.f;
	btVector3 inertia {0,0,0};

	auto ms = new btDefaultMotionState{btTransform{}};

	RigidBodyInfo bodyInfo{mass, ms, nullptr, inertia};
	rigidbody = new RigidBody{bodyInfo};

	collider = nullptr;
}

VoxelChunk::~VoxelChunk() {
	delete[] geometryData;
	delete[] rotationData;
	geometryData = nullptr;
	rotationData = nullptr;

	delete[] occlusionData;
	occlusionData = nullptr;

	if(numQuads){
		Physics::world->removeRigidBody(rigidbody);
	}

	if(auto tmsh = (btBvhTriangleMeshShape*) collider) {
		if(auto mi = tmsh->getMeshInterface())
			delete mi;

		delete tmsh;
	}

	delete rigidbody->getMotionState();
	delete rigidbody;
	rigidbody = nullptr;

	for(u32 i = 0; i < width*depth*height; i++) {
		if(blocks[i]) {
			delete blocks[i];
		}
	}

	delete[] blocks;
	blocks = nullptr;
}

static btVector3 VoxIntToVert(u32 vert) {
	vec3 offset;
	offset.x = vert & 127u;
	offset.y = (vert >> 7u) & 127u;
	offset.z = ((vert >> 14u) & 511u) * 0.5f;
	offset = vec3{offset.x, offset.z, -offset.y};
	return o2bt(offset);
}

void VoxelChunk::GenerateMesh() {
	auto manager = ChunkManager::Get();
	auto mm = &manager->mm;

	auto vinput = stbvox_get_input_description(mm);
	vinput->blocktype = geometryData;
	vinput->lighting = occlusionData;
	vinput->rotate = rotationData;

	stbvox_set_input_stride(mm, (depth+2)*(height+2), (depth+2));
	stbvox_set_input_range(mm, 1, 1, 1, width+1, height+1, depth+1);
	stbvox_set_default_mesh(mm, 0);

	stbvox_reset_buffers(mm);
	stbvox_set_buffer(mm, 0, 0, manager->vertexBuildBuffer, ChunkManager::VertexBufferSize);
	stbvox_set_buffer(mm, 0, 1, manager->faceBuildBuffer, ChunkManager::FaceBufferSize);

	if(!stbvox_make_mesh(mm)) {
		// TODO: resize and try again/continue
		// Low priority, chunks should be relatively small anyway
		logger << "Mesh generator ran out of room";
	}

	// If mesh is valid then collider is assumed to exist
	// so destroy it
	if(numQuads) {
		Physics::world->removeRigidBody(rigidbody);
		delete ((btBvhTriangleMeshShape*) collider)->getMeshInterface();
		delete collider;
		collider = nullptr;
	}

	numQuads = stbvox_get_quad_count(mm, 0);

	// If a mesh was generated, generate a new collider
	if(numQuads) {
		auto trimesh = new btTriangleMesh();
		u32* chunkVerts = (u32*)manager->vertexBuildBuffer;
		for(u64 i = 0; i < numQuads; i++) {
			btVector3 vs[] = {
				VoxIntToVert(chunkVerts[i*4+0]),
				VoxIntToVert(chunkVerts[i*4+1]),
				VoxIntToVert(chunkVerts[i*4+2]),
				VoxIntToVert(chunkVerts[i*4+3]),
			};

			trimesh->addTriangle(vs[0], vs[1], vs[2]);
			trimesh->addTriangle(vs[0], vs[2], vs[3]);
		}

		collider = new btBvhTriangleMeshShape{trimesh, true};
		collider->setUserPointer(this);

		rigidbody->setCollisionShape(collider);
		Physics::world->addRigidBody(rigidbody);
	}
}

void VoxelChunk::Update() {
	UpdateBlocks();
	// TODO: Check bordering voxels of neighbors and copy into margin
	// Otherwise AO breaks
	// Or rather notify neighbors when edges change

	// This could alternatively be done on a per block basis
	//	rather than updating every voxel in the chunk
	if(blocksDirty) {
		UpdateVoxelData();
		blocksDirty = false;
	}

	// Update collider transform
	btTransform worldTrans;
	// worldTrans.setFromOpenGLMatrix(glm::value_ptr(modelMatrix));
	worldTrans.setFromOpenGLMatrix(glm::value_ptr(glm::translate(position)));
	rigidbody->setCenterOfMassTransform(worldTrans);
}

void VoxelChunk::UpdateVoxelData() {
	for(u32 x = 0; x < width; x++)
	for(u32 y = 0; y < height; y++)
	for(u32 z = 0; z < depth; z++) {
		auto idx = 1 + z + (y+1)*(depth+2) + (x+1)*(depth+2)*(height+2);
		auto block = blocks[z + y*depth + x*depth*height];

		if(!block) {
			geometryData[idx] = 0;
			occlusionData[idx] = 255;
			continue;
		}

		auto bi = block->info;
		occlusionData[idx] = bi->doesOcclude? 0:255;
		rotationData[idx] = block->orientation;

		if(bi->RequiresIDsForRotations()) {
			geometryData[idx] = bi->voxelID + block->orientation;
		}else{
			geometryData[idx] = bi->voxelID;		
		}
	}

	voxelsDirty = true;
}

void VoxelChunk::UpdateBlocks() {
	for(u32 x = 0; x < width; x++)
	for(u32 y = 0; y < height; y++)
	for(u32 z = 0; z < depth; z++) {
		auto block = blocks[z + y*depth + x*depth*height];
		if(!block) continue;

		if(auto dyn = block->AsDynamic())
			dyn->Update();
	}
}

void VoxelChunk::PostRender() {
	for(u32 x = 0; x < width; x++)
	for(u32 y = 0; y < height; y++)
	for(u32 z = 0; z < depth; z++) {
		auto block = blocks[z + y*depth + x*depth*height];
		if(!block) continue;

		if(auto dyn = block->AsDynamic())
			dyn->PostRender();
	}
}

std::shared_ptr<VoxelChunk> VoxelChunk::GetOrCreateNeighbor(vec3 position) {
	auto manager = ChunkManager::Get();
	std::shared_ptr<ChunkNeighborhood> neigh;

	if(neigh = neighborhood.lock()) {
		for(auto& n: neigh->chunks) {
			auto ch = n.lock();
			if(!ch) continue;

			auto vxpos = ch->WorldToVoxelSpace(position);
			if(ch->InBounds(vxpos)) return ch;
		}
	}else{
		neigh = manager->CreateNeighborhood();
		SetNeighborhood(neigh);
	}

	auto vxpos = WorldToVoxelSpace(position);
	vec3 orthoDir {0};

	if(vxpos.x >=(s32)width) 		orthoDir = vec3{1,0,0};
	else if(vxpos.z >=(s32)depth) 	orthoDir = vec3{0,1,0};
	else if(vxpos.y >=(s32)height) 	orthoDir = vec3{0,0,-1};

	else if(vxpos.x < 0) 			orthoDir = vec3{-1,0,0};
	else if(vxpos.z < 0) 			orthoDir = vec3{0,-1,0};
	else if(vxpos.y < 0) 			orthoDir = vec3{0,0,1};

	auto chunk = manager->CreateChunk(width, height, depth);
	// chunk->modelMatrix = modelMatrix * glm::translate(orthoDir * vec3{width, depth, height});
	chunk->position = position + orthoDir * vec3{width, depth, height};
	chunk->SetNeighborhood(neigh);

	std::swap(orthoDir.y, orthoDir.z);
	orthoDir.y = -orthoDir.y;
	chunk->positionInNeighborhood = positionInNeighborhood + ivec3{orthoDir};

	return chunk;
}

void VoxelChunk::SetNeighborhood(std::shared_ptr<ChunkNeighborhood> n) {	
	if(auto neigh = neighborhood.lock()) {
		neigh->RemoveChunk(self.lock());
	}

	n->AddChunk(self.lock());
	neighborhood = n;
}


Block* VoxelChunk::CreateBlock(ivec3 pos, u16 id) {
	if(!InBounds(pos)) return nullptr;
	if(!id || id-1 >= BlockRegistry::blockInfoCount) return nullptr;

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;
	auto factory = BlockRegistry::blocks[id-1].factory;
	if(!factory) throw "Block " + std::to_string(id+1) + " missing factory";

	auto block = factory->Create();
	blocks[idx] = block;
	block->orientation = 0;

	if(auto dyn = block->AsDynamic()) {
		dyn->x = pos.x;
		dyn->y = pos.y;
		dyn->z = pos.z;
		dyn->chunk = this;
		
		dyn->OnPlace();
	}

	blocksDirty = true;
	return block;
}

void VoxelChunk::DestroyBlock(ivec3 pos) {
	if(!InBounds(pos)) return;

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;
	auto& block = blocks[idx];

	// TODO: Some of this should probably be deferred
	if(block) {
		if(auto dyn = block->AsDynamic())
			dyn->OnBreak();
	}

	delete block;
	block = nullptr;

	blocksDirty = true;
}

Block* VoxelChunk::GetBlock(ivec3 pos) {
	if(!InBounds(pos)) return nullptr;
	auto idx = pos.z + pos.y*depth + pos.x*depth*height;

	return blocks[idx];
}

ivec3 VoxelChunk::WorldToVoxelSpace(vec3 w) {
	// auto modelSpace = glm::inverse(modelMatrix) * vec4{w, 1};
	auto modelSpace = w - position;
	return ivec3 {
		floor(modelSpace.x-1.f),
		floor(-modelSpace.z-1.f),
		floor(modelSpace.y-1.f),
	};
}

vec3 VoxelChunk::VoxelToWorldSpace(ivec3 v) {
	auto modelSpace = vec3{v.x+1, v.z+1, -v.y-1};
	// return vec3{modelMatrix * modelSpace};
	return position + modelSpace;
}

bool VoxelChunk::InBounds(ivec3 p) {
	return !(
		(u32)p.x >= width || 
		(u32)p.y >= height || 
		(u32)p.z >= depth);
}
