#include "block.h"
#include "server.h"
#include "network.h"
#include "voxelchunk.h"
#include "serverplayer.h"
#include "chunkmanager.h"
#include "playermanager.h"

#include <chrono>
#include <thread>

static Log logger{"Server"};

using namespace std::chrono;

void Server::Run() {
	Log::SetLogFile("server.out");

	network = Network::Get();
	network->Init();
	network->Host(16660, 10);

	chunkManager = ChunkManager::Get();
	playerManager = PlayerManager::Get();
	playerIDCount = 0;
	chunkIDCount = 0;
	neighborhoodIDCount = 0;

	constexpr s32 startPlaneSize = 3;
	auto startPlaneNeigh = chunkManager->CreateNeighborhood();
	startPlaneNeigh->neighborhoodID = ++neighborhoodIDCount;
	startPlaneNeigh->position = vec3{0, -24.f, 0};
	startPlaneNeigh->rotation = quat{1, 0, 0, 0};
	// startPlaneNeigh->rotation = glm::angleAxis<f32>(PI/8.f, vec3{1,0,0});

	for(s32 cx = -startPlaneSize; cx <= startPlaneSize; cx++)
	for(s32 cz = -startPlaneSize; cz <= startPlaneSize; cz++){
		auto chunk = chunkManager->CreateChunk(24,24,24);
		chunk->SetNeighborhood(startPlaneNeigh);
		chunk->positionInNeighborhood = ivec3{cx, cz, 0};
		chunk->chunkID = ++chunkIDCount;

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(ivec3{x,y,0}, 1);

		for(u32 z = 1; z < chunk->depth; z++)
			chunk->CreateBlock(ivec3{12,12,z}, 3);
	}
	startPlaneNeigh->UpdateChunkTransforms();

	auto mNeigh = chunkManager->CreateNeighborhood();
	mNeigh->neighborhoodID = ++neighborhoodIDCount;
	mNeigh->position = vec3{0, 0, 0};
	// mNeigh->rotation = quat{1,0,0,0};
	mNeigh->rotation = glm::angleAxis<f32>(PI/4.f, vec3{0, 1, 0});
	{	auto chunk = chunkManager->CreateChunk(3,3,3);
		chunk->SetNeighborhood(mNeigh);
		chunk->chunkID = ++chunkIDCount;

		for(u8 x = 0; x < 3; x++)
		for(u8 y = 0; y < 3; y++)
		for(u8 z = 0; z < 3; z++)
			chunk->CreateBlock(ivec3{x,y,z}, 3);
	}

	mNeigh->rotation = glm::angleAxis<f32>(PI/4.f, vec3{.707f, 0, .707f});
	mNeigh->UpdateChunkTransforms();

	logger << "Init";

	Packet packet;
	while(true) {
		network->Update();

		while(network->GetPacket(&packet)) {
			u8 type = packet.ReadType();

			switch(type) {
			case ID_NEW_INCOMING_CONNECTION: OnPlayerConnect(packet.fromGUID); break;
			case ID_DISCONNECTION_NOTIFICATION: OnPlayerDisonnect(packet.fromGUID); break;
			case ID_CONNECTION_LOST: OnPlayerLostConnection(packet.fromGUID); break;

			case PacketType::UpdatePlayerState: OnPlayerStateUpdate(packet); break;
			case PacketType::SetBlock: OnSetBlock(packet); break;
			case PacketType::PlayerInteract: OnInteract(packet); break;
			case PacketType::ChunkDownload:
				for(auto& chunk: chunkManager->chunks){
					SendChunkContents(chunk, packet.fromGUID);
				}
				break;
			}
		}

		playerManager->Update();

		// static f32 t = 0;
		// mNeigh->rotation = glm::angleAxis<f32>((t += 0.01f), vec3{.707f, 0, .707f});
		// mNeigh->rotation = glm::angleAxis<f32>(0*PI/2.f, vec3{.707f, 0, .707f});
		// mNeigh->UpdateChunkTransforms();

		// SendNeighborhoodTransform(mNeigh);

		std::this_thread::sleep_for(milliseconds{10});
	}
}

#include <raknet/RakPeerInterface.h>

void Server::OnPlayerConnect(NetworkGUID guid) {
	u16 playerID = ++playerIDCount;
	guidToPlayerID[guid] = playerID;
	playerManager->AddPlayer(std::make_shared<ServerPlayer>(), playerID);

	auto sa = network->peer->GetSystemAddressFromGuid(guid);
	logger << "Client " << playerID << " connected [" << sa.ToString() << "]";

	Packet packet;
	packet.WriteType(PacketType::RemoteJoin);
	packet.Write(playerID);
	network->Broadcast(packet, guid);

	for(auto& chunk: chunkManager->chunks) {
		SendNewChunk(chunk, guid);
	}

	// NOTE: If a ChunkDownload packet arrives before its corresponding
	//	NewChunk packet, it will be discarded on the client side
	for(auto& chunk: chunkManager->chunks){
		SendChunkContents(chunk, guid);
	}
}

void Server::OnPlayerDisonnect(NetworkGUID guid) {
	// playerID 0 is invalid
	auto playerID = guidToPlayerID[guid];
	if(!playerID) return;

	logger << "Client " << playerID << " disconnected";

	Packet packet;
	packet.WriteType(PacketType::RemoteLeave);
	packet.Write(playerID);
	network->Broadcast(packet, guid);
}

void Server::OnPlayerLostConnection(NetworkGUID guid) {
	// playerID 0 is invalid
	auto playerID = guidToPlayerID[guid];
	if(!playerID) return;

	Packet packet;
	packet.WriteType(PacketType::RemoteLeave);
	packet.Write(playerID);

	logger << "Client " << playerID << " lost connection";
}

void Server::OnPlayerStateUpdate(Packet& p) {
	auto playerID = guidToPlayerID[p.fromGUID];
	if(!playerID) return;

	auto player = playerManager->GetPlayer(playerID);
	if(!player) return;

	vec3 pos, vel;
	quat ori;

	p.Read(pos);
	p.Read(vel);
	p.Read(ori);
	player->SetPosition(pos);
	player->SetVelocity(vel);
	player->SetOrientation(ori);

	// Copy packet and forward to everyone else
	Packet np;
	np.WriteType(PacketType::UpdatePlayerState);
	np.Write<u16>(playerID);
	np.Write(pos);
	np.Write(vel);
	np.Write(ori);
	network->Broadcast(np, p.fromGUID);
}

void Server::OnSetBlock(Packet& p) {
	u16 chunkID, blockType;
	u8 orientation;
	ivec3 vxPos;

	p.Read(chunkID);
	p.Read(vxPos);
	p.Read(blockType);

	orientation = blockType & 3;
	blockType >>= 2;

	auto ch = chunkManager->GetChunk(chunkID);
	if(!ch) {
		logger << "Client tried to modify chunk that isn't known to server";
		return;
	}

	if(!ch->InBounds(vxPos)) {
		auto neigh = ch->neighborhood.lock();
		if(!neigh) {
			neigh = chunkManager->CreateNeighborhood();
			neigh->neighborhoodID = ++neighborhoodIDCount;
			ch->SetNeighborhood(neigh);
			SendSetNeighborhood(ch);
		}

		auto nchunk = ch->GetOrCreateNeighborContaining(vxPos);
		if(!nchunk) {
			logger << "Neighbor chunk creation failed!";
			return;
		}

		// If chunkID is zero, it must be new
		if(!nchunk->chunkID){
			nchunk->chunkID = ++chunkIDCount;
			SendNewChunk(nchunk);
		}

		vec3 world = ch->VoxelToWorldSpace(vxPos);
		vxPos = nchunk->WorldToVoxelSpace(world);

		ch = nchunk;
		chunkID = nchunk->chunkID;
	}

	if(!blockType) {
		ch->DestroyBlock(vxPos);

	}else{
		auto block = ch->CreateBlock(vxPos, blockType);
		if(!block) {
			logger << "Block creation failed for block type " << blockType;
			return;
		}else{
			block->orientation = orientation;
		}
	}

	// Packet needs to be copied because vxPos and chunkID can change
	Packet np;
	np.WriteType(PacketType::SetBlock);
	np.Write(chunkID);
	np.Write(vxPos);
	np.Write<u16>(blockType << 2 | orientation);

	np.reliability = RELIABLE_ORDERED;

	// Send to all including sender
	network->Broadcast(np);
}

void Server::OnInteract(Packet& p) {
	u16 chunkID;
	ivec3 vxPos;

	p.Read(chunkID);
	p.Read(vxPos);

	auto ch = chunkManager->GetChunk(chunkID);
	if(!ch) {
		logger << "Player trying to interact with non-existent chunk";
		return;
	}

	auto blk = ch->GetBlock(vxPos);
	if(!blk) {
		logger << "Player trying to interact with non-existent block";
	}

	if(auto dyn = blk->AsDynamic())
		dyn->OnInteract();
}

void Server::SendNewChunk(std::shared_ptr<VoxelChunk> vc, NetworkGUID guid) {
	auto neigh = vc->neighborhood.lock();

	Packet packet;
	packet.WriteType(PacketType::NewChunk);
	packet.Write(vc->chunkID);
	packet.Write<u16>(neigh?neigh->neighborhoodID:0);
	packet.Write<u8>(vc->width);
	packet.Write<u8>(vc->height);
	packet.Write<u8>(vc->depth);
	packet.Write(vc->position);
	packet.Write(vc->rotation);

	packet.reliability = RELIABLE_ORDERED;
	network->Send(packet, guid);
}

void Server::SendSetNeighborhood(std::shared_ptr<VoxelChunk> vc, NetworkGUID guid) {
	auto neigh = vc->neighborhood.lock();

	Packet packet;
	packet.WriteType(PacketType::SetChunkNeighborhood);
	packet.Write(vc->chunkID);
	packet.Write<u16>(neigh?neigh->neighborhoodID:0);
	packet.Write(vc->positionInNeighborhood);

	network->Send(packet, guid);
}

void Server::SendChunkContents(std::shared_ptr<VoxelChunk> vc, NetworkGUID guid) {
	if(vc->width > 32
	|| vc->depth > 32
	|| vc->height > 32) {
		logger << "Cannot send chunks of size > 32x32x32";
		return;
	}

	constexpr u16 blockLimit = 245;

	auto blocks = vc->blocks;
	u16 w = vc->width;
	u16 h = vc->height;
	u16 d = vc->depth;
	u16 numBlocks = w*h*d;
	std::vector<u16> packetInfo(numBlocks, 0);

	for(u16 i = 0; i < numBlocks; i++) {
		auto b = blocks[i];
		if(!b) continue;

		auto bID = b->info->blockID;
		packetInfo[i] = bID << 2 | (b->orientation & 3);
	}

	// TODO: Compression could happen here

	Packet p;
	p.reliability = RELIABLE_ORDERED;

	u16 remaining = numBlocks;
	while(remaining >= blockLimit) {
		u16 offset = numBlocks - remaining;

		p.Reset();
		p.WriteType(PacketType::ChunkDownload);
		p.Write<u16>(vc->chunkID);
		p.Write<u16>(offset);
		p.Write<u8>(blockLimit);

		for(u16 i = 0; i < blockLimit; i++)
			p.Write<u16>(packetInfo[i+offset]);

		network->Send(p, guid);

		remaining -= blockLimit;
	}

	if(remaining > 0) {
		u16 offset = numBlocks - remaining;

		p.Reset();
		p.WriteType(PacketType::ChunkDownload);
		p.Write<u16>(vc->chunkID);
		p.Write<u16>(offset);
		p.Write<u8>(remaining);

		for(u16 i = 0; i < remaining; i++)
			p.Write<u16>(packetInfo[i+offset]);

		network->Send(p, guid);
	}
}

void Server::SendNeighborhoodTransform(std::shared_ptr<ChunkNeighborhood> neigh, NetworkGUID guid) {
	Packet p;
	p.WriteType(PacketType::SetNeighborhoodTransform);
	p.Write<u16>(neigh->neighborhoodID);
	p.Write(neigh->position);
	p.Write(neigh->rotation);

	network->Send(p, guid);
}
