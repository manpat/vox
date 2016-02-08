#include "chunkmanager.h"
#include "quadbuffer.h"
#include "voxelchunk.h"
#include "physics.h"
#include "shader.h"
#include "block.h"

#include "stb_voxel_render.h"

static Log logger{"VoxelChunk"};

VoxelChunk::VoxelChunk(ChunkManager* cm, u32 w, u32 h, u32 d) 
	: manager{cm}, width{w}, height{h}, depth{d} {

	u64 size = (width+2)*(height+2)*(depth+2);
	geometryData = new u8[size];
	rotationData = new u8[size];
	occlusionData = new u8[size];

	blocks = new Block*[width*height*depth];
	
	memset(geometryData, 0, size);
	memset(rotationData, 0, size);
	memset(occlusionData, 255, size);

	memset(blocks, 0, width*height*depth * sizeof(Block*));

	vertexBO = faceBO = faceTex = 0;
	numQuads = 0;
	blocksDirty = false;
	voxelsDirty = true;

	modelMatrix = mat4(1.f);

	btScalar mass = 0.f;
	btVector3 inertia {0,0,0};

	auto ms = new btDefaultMotionState{btTransform{}};

	RigidBodyInfo bodyInfo{mass, ms, nullptr, inertia};
	rigidbody = new RigidBody{bodyInfo};

	collider = nullptr;
}

VoxelChunk::~VoxelChunk() {
	glDeleteBuffers(1, &vertexBO);
	glDeleteBuffers(1, &faceBO);
	glDeleteTextures(1, &faceTex);

	delete[] geometryData;
	delete[] occlusionData;
	delete[] rotationData;
	geometryData = nullptr;
	rotationData = nullptr;
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

void VoxelChunk::UploadMesh() {
	if(!numQuads) return;

	if(!vertexBO) glGenBuffers(1, &vertexBO);
	if(!faceBO) glGenBuffers(1, &faceBO);
	if(!faceTex) glGenTextures(1, &faceTex);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glBufferData(GL_ARRAY_BUFFER, numQuads*4*sizeof(u32), manager->vertexBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_TEXTURE_BUFFER, faceBO);
	glBufferData(GL_TEXTURE_BUFFER, numQuads*sizeof(u32), manager->faceBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	glBindTexture(GL_TEXTURE_BUFFER, faceTex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8UI, faceBO);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void VoxelChunk::Render(ShaderProgram* program) {
	if(voxelsDirty) {
		GenerateMesh();
		UploadMesh();
		voxelsDirty = false;
	}

	if(!numQuads) return;

	QuadElementBuffer::SetNumQuads(numQuads);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 4, nullptr);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, faceTex);
	glUniform1i(program->GetUniform("facearray"), 0);

	static mat4 coordinateCorrection = glm::rotate<f32>(-PI/2.f, vec3{1,0,0});
	glUniformMatrix4fv(program->GetUniform("model"), 1, false, 
		glm::value_ptr(modelMatrix * coordinateCorrection));

	QuadElementBuffer::Draw(numQuads);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void VoxelChunk::Update() {
	// TODO: A way for blocks to set dirty flag
	UpdateBlocks();

	blocksDirty = true;
	if(blocksDirty) {
		UpdateVoxelData();
		blocksDirty = false;
	}

	// Update collider transform
	btTransform worldTrans;
	worldTrans.setFromOpenGLMatrix(glm::value_ptr(modelMatrix));
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
		if(!block || !block->info->dynamic) continue;

		block->Update();
	}
}
void VoxelChunk::PostRender() {
	for(u32 x = 0; x < width; x++)
	for(u32 y = 0; y < height; y++)
	for(u32 z = 0; z < depth; z++) {
		auto block = blocks[z + y*depth + x*depth*height];
		if(!block || !block->info->dynamic) continue;

		block->PostRender();
	}
}

Block* VoxelChunk::CreateBlock(ivec3 pos, u16 id) {
	if((u32)pos.x >= width || (u32)pos.y >= height || (u32)pos.z >= depth) return nullptr;
	if(!id || id-1 >= BlockRegistry::blockInfoCount) return nullptr;

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;
	auto factory = BlockRegistry::blocks[id-1].factory;
	if(!factory) throw "Block " + std::to_string(id+1) + " missing factory";

	auto block = factory->Create();
	blocks[idx] = block;
	block->orientation = 0;
	block->x = pos.x;
	block->y = pos.y;
	block->z = pos.z;
	block->chunk = this;

	block->OnPlace();

	blocksDirty = true;
	return block;
}

void VoxelChunk::DestroyBlock(ivec3 pos) {
	if((u32)pos.x >= width || (u32)pos.y >= height || (u32)pos.z >= depth) return;

	auto idx = pos.z + pos.y*depth + pos.x*depth*height;
	auto& block = blocks[idx];

	// TODO: Some of this should really be deferred
	if(block) {
		block->OnBreak();
	}

	delete block;
	block = nullptr;

	blocksDirty = true;
}

Block* VoxelChunk::GetBlock(ivec3 pos) {
	if((u32)pos.x >= width || (u32)pos.y >= height || (u32)pos.z >= depth) return nullptr;
	auto idx = pos.z + pos.y*depth + pos.x*depth*height;

	return blocks[idx];
}

ivec3 VoxelChunk::WorldToVoxelSpace(vec3 w) {
	auto modelSpace = glm::inverse(modelMatrix) * vec4{w, 1};
	return ivec3 {
		floor(modelSpace.x-1.f),
		floor(-modelSpace.z-1.f),
		floor(modelSpace.y-1.f),
	};
}

vec3 VoxelChunk::VoxelToWorldSpace(ivec3 v) {
	vec4 modelSpace = vec4{v.x+1, v.z+1, -v.y-1, 1};
	return vec3{modelMatrix * modelSpace};
}

