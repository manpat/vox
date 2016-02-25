#include "chunkmanager.h"
#include "physics.h"
#include "block.h"
#include "chunk.h"

#include "stb_voxel_render.h"

static Log logger{"Chunk"};

Chunk::Chunk(u8 w, u8 h, u8 d) 
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
	rotation = quat{1,0,0,0};

	btScalar mass = 0.f;
	btVector3 inertia {0,0,0};

	// TODO: When shit starts moving, chunks will probably need 
	//	their own MotionState
	auto ms = new btDefaultMotionState{btTransform{}};

	RigidBodyInfo bodyInfo{mass, ms, nullptr, inertia};
	rigidbody = new RigidBody{bodyInfo};

	collider = nullptr;
}

Chunk::~Chunk() {
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

	for(u32 i = 0; i < (u32)width*depth*height; i++) {
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

void Chunk::GenerateMesh() {
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

void Chunk::Update() {
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
	worldTrans.setFromOpenGLMatrix(glm::value_ptr(glm::translate(position) * glm::mat4_cast(rotation)));
	rigidbody->setCenterOfMassTransform(worldTrans);
}

void Chunk::UpdateVoxelData() {
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

		auto bi = block->GetInfo();
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

void Chunk::UpdateBlocks() {
	// TODO: If this ever becomes a problem, dynamic blocks
	//	could be stored in another list
	for(u32 i = 0; i < (u32)width*depth*height; i++) {
		auto block = blocks[i];
		if(!block) continue;

		if(auto dyn = block->AsDynamic())
			dyn->Update();
	}
}

void Chunk::PostRender() {
	// TODO: If this ever becomes a problem, dynamic blocks
	//	could be stored in another list
	for(u32 i = 0; i < (u32)width*depth*height; i++) {
		auto block = blocks[i];
		if(!block) continue;

		if(auto dyn = block->AsDynamic())
			dyn->PostRender();
	}
}

std::shared_ptr<Chunk> Chunk::GetOrCreateNeighborContaining(ivec3 vxpos) {
	auto manager = ChunkManager::Get();
	std::shared_ptr<ChunkNeighborhood> neigh;

	if(neigh = neighborhood.lock()) {
		vec3 world = VoxelToWorldSpace(vxpos);

		// TODO: It might be a good idea to use positionInNeighborhood for this
		// Low priority as this won't get called often
		for(auto& n: neigh->chunks) {
			auto ch = n.lock();
			if(!ch) continue;

			auto vxpos = ch->WorldToVoxelSpace(world);
			if(ch->InBounds(vxpos)) return ch;
		}
	}else{
		neigh = manager->CreateNeighborhood();
		SetNeighborhood(neigh);
	}

	ivec3 orthoDir {0};

	if(vxpos.x >=(s32)width) 		orthoDir = ivec3{1,0,0};
	else if(vxpos.y >=(s32)height) 	orthoDir = ivec3{0,1,0};
	else if(vxpos.z >=(s32)depth) 	orthoDir = ivec3{0,0,1};

	else if(vxpos.x < 0) 			orthoDir = ivec3{-1,0,0};
	else if(vxpos.y < 0) 			orthoDir = ivec3{0,-1,0};
	else if(vxpos.z < 0) 			orthoDir = ivec3{0,0,-1};

	auto chunk = manager->CreateChunk(width, height, depth);

	chunk->SetNeighborhood(neigh);
	chunk->positionInNeighborhood = positionInNeighborhood + orthoDir;

	neigh->UpdateChunkTransform(chunk);

	return chunk;
}

void Chunk::SetNeighborhood(std::shared_ptr<ChunkNeighborhood> n) {	
	if(auto neigh = neighborhood.lock()) {
		neigh->RemoveChunk(self.lock());
	}

	n->AddChunk(self.lock());
	neighborhood = n;
}


Block* Chunk::CreateBlock(ivec3 pos, u16 id) {
	if(!InBounds(pos)) return nullptr;

	auto blockInfo = BlockRegistry::GetBlockInfo(id);
	if(!blockInfo) return nullptr;

	auto factory = blockInfo->factory;
	if(!factory) throw "Block " + std::to_string(id+1) + " missing factory";

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;

	// TODO: Is this good enough?
	// If a block already exists destroy it
	if(auto block = blocks[idx]) {
		if(auto dyn = block->AsDynamic())
			dyn->OnBreak();

		auto factory = block->GetFactory();
		if(factory) {
			factory->Destroy(block);
		}
	}

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

void Chunk::DestroyBlock(ivec3 pos) {
	if(!InBounds(pos)) return;

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;
	auto& block = blocks[idx];

	// TODO: Some of this should probably be deferred
	if(block) {
		if(auto dyn = block->AsDynamic())
			dyn->OnBreak();

		auto factory = block->GetFactory();
		if(factory) {
			factory->Destroy(block);
		}else{
			auto bi = block->GetInfo();
			logger << "Tried to destroy block with no factory!";
			logger << (bi? bi->name : "<null blockinfo>");
		}
	}

	block = nullptr;

	blocksDirty = true;
}

Block* Chunk::GetBlock(ivec3 pos) {
	if(!InBounds(pos)) return nullptr;
	auto idx = pos.z + pos.y*depth + pos.x*depth*height;

	return blocks[idx];
}

ivec3 Chunk::WorldToVoxelSpace(vec3 w) {
	auto modelSpace = glm::inverse(rotation) * (w - position);
	return ivec3 {
		floor(modelSpace.x-1.f),
		floor(-modelSpace.z-1.f),
		floor(modelSpace.y-1.f),
	};
}

vec3 Chunk::VoxelToWorldSpace(ivec3 v) {
	// Returned coordinate is center of voxel
	auto modelSpace = rotation * vec3{v.x+1.5, v.z+1.5, -v.y-1.5};
	return position + modelSpace;
}

bool Chunk::InBounds(ivec3 p) {
	return !(
		(u32)p.x >= width || 
		(u32)p.y >= height || 
		(u32)p.z >= depth);
}
