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

	constexpr s32 startPlaneSize = 1;
	auto startPlaneNeigh = chunkManager->CreateNeighborhood();
	startPlaneNeigh->neighborhoodID = ++neighborhoodIDCount;

	for(s32 cx = -startPlaneSize; cx <= startPlaneSize; cx++)
	for(s32 cz = -startPlaneSize; cz <= startPlaneSize; cz++){
		auto chunk = chunkManager->CreateChunk(24,24,24);
		chunk->SetNeighborhood(startPlaneNeigh);
		chunk->positionInNeighborhood = ivec3{cx, cz, 0};
		chunk->position = vec3{cx * 24.f, -24, -cz * 24.f};
		chunk->chunkID = ++chunkIDCount;

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(ivec3{x,y,0}, 1);

		for(u32 z = 1; z < chunk->depth; z++)
			chunk->CreateBlock(ivec3{12,12,z}, 3);
	}

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
			}
		}

		playerManager->Update();

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

	logger << "SetBlock [" << chunkID << "] " << vxPos;

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
		if(!block) logger << "Block creation failed for block type " << blockType;
		else block->orientation = orientation;
	}

	// Packet needs to be copied because vxPos and chunkID can change
	Packet np;
	np.WriteType(PacketType::SetBlock);
	np.Write(chunkID);
	np.Write(vxPos);
	np.Write<u16>(blockType << 2 | orientation);

	// Send to all including sender
	network->Broadcast(np);
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

	Packet p;
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