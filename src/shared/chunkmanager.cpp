#include "chunkmeshbuilder.h"
#include "chunkmanager.h"
#include "chunk.h"
#include "block.h"

static Log logger{"ChunkManager"};

std::shared_ptr<ChunkManager> ChunkManager::Get() {
	static std::weak_ptr<ChunkManager> wp;
	std::shared_ptr<ChunkManager> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<ChunkManager>());

	return p;
}

ChunkManager::ChunkManager() {
	meshBuilder = std::make_shared<ChunkMeshBuilder>();
}
ChunkManager::~ChunkManager() {}

std::shared_ptr<Chunk> ChunkManager::CreateChunk(u32 w, u32 h, u32 d) {
	auto nchunk = std::make_shared<Chunk>(w,h,d);
	chunks.push_back(nchunk);

	nchunk->chunkID = 0;
	nchunk->position = vec3{0.f};
	nchunk->self = nchunk;
	return nchunk;
}

std::shared_ptr<ChunkNeighborhood> ChunkManager::CreateNeighborhood() {
	auto nhood = std::make_shared<ChunkNeighborhood>();
	neighborhoods.emplace_back(nhood);
	return nhood;
}

void ChunkManager::Update() {
	for(auto& vc: chunks) {
		vc->Update();
	}
}

std::shared_ptr<Chunk> ChunkManager::GetChunk(u16 id) {
	auto it = std::find_if(chunks.begin(), chunks.end(), [id](const std::shared_ptr<Chunk>& ch) {
		return id == ch->chunkID;
	});

	if(it == chunks.end()) return nullptr;
	return *it;
}

void ChunkManager::DestroyChunk(u16 id) {
	auto it = std::find_if(chunks.begin(), chunks.end(), [id](const std::shared_ptr<Chunk>& ch) {
		return id == ch->chunkID;
	});

	if(it == chunks.end()) return;
	chunks.erase(it);
}

std::shared_ptr<ChunkNeighborhood> ChunkManager::GetNeighborhood(u16 id) {
	auto it = std::find_if(neighborhoods.begin(), neighborhoods.end(), 
		[id](const std::shared_ptr<ChunkNeighborhood>& ch) {
			return id == ch->neighborhoodID;
	});

	if(it == neighborhoods.end()) return nullptr;
	return *it;
}

/*
	                                                                                                                                       
	888b      88            88             88          88                                 88                                           88  
	8888b     88            ""             88          88                                 88                                           88  
	88 `8b    88                           88          88                                 88                                           88  
	88  `8b   88  ,adPPYba, 88  ,adPPYb,d8 88,dPPYba,  88,dPPYba,   ,adPPYba,  8b,dPPYba, 88,dPPYba,   ,adPPYba,   ,adPPYba,   ,adPPYb,88  
	88   `8b  88 a8P_____88 88 a8"    `Y88 88P'    "8a 88P'    "8a a8"     "8a 88P'   "Y8 88P'    "8a a8"     "8a a8"     "8a a8"    `Y88  
	88    `8b 88 8PP""""""" 88 8b       88 88       88 88       d8 8b       d8 88         88       88 8b       d8 8b       d8 8b       88  
	88     `8888 "8b,   ,aa 88 "8a,   ,d88 88       88 88b,   ,a8" "8a,   ,a8" 88         88       88 "8a,   ,a8" "8a,   ,a8" "8a,   ,d88  
	88      `888  `"Ybbd8"' 88  `"YbbdP"Y8 88       88 8Y"Ybbd8"'   `"YbbdP"'  88         88       88  `"YbbdP"'   `"YbbdP"'   `"8bbdP"Y8  
	                            aa,    ,88                                                                                                 
	                             "Y8bbdP"                                                                                                  
*/
void ChunkNeighborhood::AddChunk(std::shared_ptr<Chunk> c) {
	if(!chunks.size()) {
		chunkSize = ivec3{c->width, c->height, c->depth};
		c->positionInNeighborhood = ivec3{0};
	}

	chunks.emplace_back(c);
}

void ChunkNeighborhood::RemoveChunk(std::shared_ptr<Chunk> c) {
	auto endIt = std::remove_if(chunks.begin(), chunks.end(), 
		[&c](const std::weak_ptr<Chunk>& wn) {
			return wn.lock() == c;
	});

	chunks.erase(endIt, chunks.end());
}

void ChunkNeighborhood::UpdateChunkTransforms() {
	for(auto& wch: chunks) {
		auto ch = wch.lock();
		if(!ch) continue;

		UpdateChunkTransform(ch);
	}
}

void ChunkNeighborhood::UpdateChunkTransform(std::shared_ptr<Chunk> ch) {
	auto offset = vec3{chunkSize * ch->positionInNeighborhood};
	std::swap(offset.y, offset.z);
	offset.z = -offset.z;

	ch->position = position + rotation * offset;
	ch->rotation = rotation;
}
