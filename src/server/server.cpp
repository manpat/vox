#include "chunk.h"
#include "block.h"
#include "server.h"
#include "network.h"
#include "serverplayer.h"
#include "chunkmanager.h"
#include "playermanager.h"

#include <chrono>
#include <thread>

static Log logger{"Server"};

using namespace std::chrono;

void Server::Run() {
	Log::SetLogFile("server.out");
	BlockRegistry::InitBlockInfo();

	network = Network::Get();
	network->Init();
	network->Host(16660, 10);

	chunkManager = ChunkManager::Get();
	playerManager = PlayerManager::Get();
	neighborhoodIDCount = 0;
	playerIDCount = 0;
	chunkIDCount = 0;

	// TEMPORARY
	// Initial chunk creation should be done on game begin,
	// 	which isn't a concept yet.
	constexpr s32 startPlaneSize = 5;
	auto startPlaneNeigh = chunkManager->CreateNeighborhood();
	startPlaneNeigh->neighborhoodID = ++neighborhoodIDCount;
	startPlaneNeigh->position = vec3{0, -24.f, 0};
	startPlaneNeigh->rotation = quat{1, 0, 0, 0};

	for(s32 cx = -startPlaneSize; cx <= startPlaneSize; cx++)
	for(s32 cz = -startPlaneSize; cz <= startPlaneSize; cz++){
		auto chunk = chunkManager->CreateChunk(24,24,24);
		chunk->SetNeighborhood(startPlaneNeigh);
		chunk->positionInNeighborhood = ivec3{cx, cz, 0};
		chunk->chunkID = ++chunkIDCount;

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(ivec3{x,y,0}, "steel");

		for(u32 z = 1; z < chunk->depth; z++)
			chunk->CreateBlock(ivec3{12,12,z}, "lightthing");
	}
	startPlaneNeigh->UpdateChunkTransforms();

	auto mNeigh = chunkManager->CreateNeighborhood();
	mNeigh->neighborhoodID = ++neighborhoodIDCount;
	mNeigh->position = vec3{0, 0, 0};
	mNeigh->rotation = glm::angleAxis<f32>(PI/4.f, vec3{0, 1, 0});
	{	auto chunk = chunkManager->CreateChunk(3,3,3);
		chunk->SetNeighborhood(mNeigh);
		chunk->chunkID = ++chunkIDCount;

		for(u8 x = 0; x < 3; x++)
		for(u8 y = 0; y < 3; y++)
		for(u8 z = 0; z < 3; z++)
			chunk->CreateBlock(ivec3{x,y,z}, "lightthing");
	}
	// TEMPORARY
	
	logger << "Init";

	Packet packet;
	while(true) {
		network->Update();

		// TODO: Instead of dispatching player/block updates as they come in,
		//	update only on serverside, then send out state updates at regular interval
		// This NEEDS to happen at some point
		// High-frequency updates should only be sent to players in range
		// Neighborhood transform updates need to be sent automatically

		while(network->GetPacket(&packet)) {
			u8 type = packet.ReadType();

			switch(type) {
			case ID_NEW_INCOMING_CONNECTION: OnPlayerConnect(packet.guid); break;
			case ID_DISCONNECTION_NOTIFICATION: OnPlayerDisonnect(packet.guid); break;
			case ID_CONNECTION_LOST: OnPlayerLostConnection(packet.guid); break;

			case PacketType::UpdatePlayerState: OnPlayerStateUpdate(packet); break;
			case PacketType::SetBlock: OnSetBlock(packet); break;
			case PacketType::PlayerInteract: OnInteract(packet); break;
			case PacketType::ChunkDownload:
				for(auto& chunk: chunkManager->chunks){
					SendChunkContents(chunk, packet.guid);
				}
				break;
			}
		}

		playerManager->Update();

		// Send state updates for all players
		// TODO: Limit sending packets to within a sector
		for(auto ply: playerManager->players) {
			packet.Reset();
			packet.WriteType(PacketType::UpdatePlayerState);
			packet.Write<u16>(ply->playerID);
			packet.Write(ply->GetPosition());
			packet.Write(ply->GetVelocity());
			packet.Write(ply->GetOrientation());
			packet.Write(ply->GetEyeOrientation());

			packet.reliability = UNRELIABLE_SEQUENCED;
			packet.priority = LOW_PRIORITY;
			network->Broadcast(packet, ply->GetGUID());
		}

		// TEMPORARY
		static f32 t = 0;
		mNeigh->rotation = glm::angleAxis<f32>((t += 0.01f), glm::normalize(vec3{1,0,1}));

		// Move rotation origin to center of chunk
		mNeigh->position = vec3{0, -10, -20} + mNeigh->rotation * -vec3{2.5, 2.5,-2.5};
		mNeigh->UpdateChunkTransforms();

		SendNeighborhoodTransform(mNeigh);
		// TEMPORARY

		std::this_thread::sleep_for(milliseconds{50});
	}
}

// This is for GetSystemAddressFromGuid
#include <raknet/RakPeerInterface.h>

void Server::OnPlayerConnect(NetworkGUID guid) {
	u16 playerID = ++playerIDCount;
	guidToPlayerID[guid] = playerID;
	auto player = std::make_shared<ServerPlayer>();
	player->guid = guid;

	playerManager->AddPlayer(player, playerID);

	auto sa = network->peer->GetSystemAddressFromGuid(guid);
	logger << "Client " << playerID << " connected [" << sa.ToString() << "]";

	// Notify players of a new player
	Packet packet;
	packet.WriteType(PacketType::RemoteJoin);
	packet.Write(playerID);
	packet.Write<u8>(0);
	network->Broadcast(packet, guid);

	// Inform new player of existing players
	for(auto& ply: playerManager->players) {
		if(ply->playerID == playerID) continue;

		packet.Reset();
		packet.WriteType(PacketType::RemoteJoin);
		packet.Write(ply->playerID);
		packet.Write<u8>(1);
		network->Send(packet, guid);
	}

	// Inform player of what chunks exist
	// TODO: Limit this to sector/range; be smarter
	for(auto& chunk: chunkManager->chunks) {
		SendNewChunk(chunk, guid);
	}

	// Notify player of actual neighborhood transforms
	for(auto& neigh: chunkManager->neighborhoods){
		SendNeighborhoodTransform(neigh, guid);
	}

	// Send the contents of the just sent chunks to player
	// TODO: Same as before, be smarter
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

	playerManager->RemovePlayer(playerID);

	// Inform players of player disconnect
	Packet packet;
	packet.WriteType(PacketType::RemoteLeave);
	packet.Write(playerID);
	packet.Write<u8>(0);
	network->Broadcast(packet, guid);

	logger << "Client " << playerID << " disconnected";
}

void Server::OnPlayerLostConnection(NetworkGUID guid) {
	// playerID 0 is invalid
	auto playerID = guidToPlayerID[guid];
	if(!playerID) return;

	playerManager->RemovePlayer(playerID);

	// Inform players of player disconnect
	Packet packet;
	packet.WriteType(PacketType::RemoteLeave);
	packet.Write(playerID);
	packet.Write<u8>(1);

	logger << "Client " << playerID << " lost connection";
}

void Server::OnPlayerStateUpdate(Packet& p) {
	auto playerID = guidToPlayerID[p.guid];
	if(!playerID) return;

	auto player = playerManager->GetPlayer(playerID);
	if(!player) return;

	vec3 pos, vel;
	quat ori, eyeOri;

	p.Read(pos);
	p.Read(vel);
	p.Read(ori);
	p.Read(eyeOri);

	// Save new player state 
	player->SetPosition(pos);
	player->SetVelocity(vel);
	player->SetOrientation(ori);
	player->SetEyeOrientation(eyeOri);
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

	auto playerID = guidToPlayerID[p.guid];
	if(!playerID) {
		logger << "Unknown player tried to set block";
		return;
	}

	auto ch = chunkManager->GetChunk(chunkID);
	if(!ch) {
		logger << "Client tried to modify chunk that isn't known to server";
		return;
	}

	// If the client tries to create a block outside 
	//	the boundary of a chunk
	if(blockType && !ch->InBounds(vxPos)) {
		// Check if this chunk is part of a neighborhood
		//	and if not, create one 
		auto neigh = ch->neighborhood.lock();
		if(!neigh) {
			neigh = chunkManager->CreateNeighborhood();
			neigh->neighborhoodID = ++neighborhoodIDCount;
			ch->SetNeighborhood(neigh);
			SendSetNeighborhood(ch);
		}

		// Try to get or create a neighboring chunk 
		//	containing the requested block
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

		// Get the new position of the block relative
		//	to the new chunk
		vec3 world = ch->VoxelToWorldSpace(vxPos);
		vxPos = nchunk->WorldToVoxelSpace(world);

		ch = nchunk;
		chunkID = nchunk->chunkID;
	}

	// blockType 0 is invalid, so destroy the block at vxPos
	if(!blockType) {
		ch->DestroyBlock(vxPos, playerID);

	}else{
		auto block = ch->CreateBlock(vxPos, blockType, playerID);
		if(!block) {
			logger << "Block creation failed for block type " << blockType;
			return;
		}else{
			block->orientation = orientation;
		}
	}

	// Notify all players of block change.
	// Packet needs to be copied because vxPos and chunkID can change
	// TODO: Instead of sending packets immediately, record into buffer and send 
	//	block updates in bulk
	Packet np;
	np.WriteType(PacketType::SetBlock);
	np.Write(chunkID);
	np.Write(vxPos);
	np.Write<u16>(blockType << 2 | (orientation & 3));

	np.reliability = RELIABLE_ORDERED;

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
		return;
	}

	auto playerID = guidToPlayerID[p.guid];
	if(!playerID) {
		logger << "Unknown player tried to interact with a block";
		return;
	}

	if(auto dyn = blk->dynamic){
		dyn->OnInteract(playerID);
	}
}

void Server::SendNewChunk(std::shared_ptr<Chunk> vc, NetworkGUID guid) {
	auto neigh = vc->neighborhood.lock();
	auto neighID = neigh?neigh->neighborhoodID:0;

	Packet packet;
	packet.WriteType(PacketType::NewChunk);
	packet.Write(vc->chunkID);
	packet.Write<u16>(neighID);
	packet.Write<u8>(vc->width);
	packet.Write<u8>(vc->height);
	packet.Write<u8>(vc->depth);

	// If chunk has a neighborhood send it's 
	//	positionInNeighborhood. Chunk position and 
	//	rotation will be calculated clientside based 
	//	on neighborhood transform
	if(neighID) {
		packet.Write(vc->positionInNeighborhood);

	}else{
		packet.Write(vc->position);
		packet.Write(vc->rotation);
	}

	packet.reliability = RELIABLE_ORDERED;
	network->Send(packet, guid);
}

void Server::SendSetNeighborhood(std::shared_ptr<Chunk> vc, NetworkGUID guid) {
	auto neigh = vc->neighborhood.lock();

	Packet packet;
	packet.WriteType(PacketType::SetChunkNeighborhood);
	packet.Write(vc->chunkID);
	packet.Write<u16>(neigh?neigh->neighborhoodID:0);
	packet.Write(vc->positionInNeighborhood);

	network->Send(packet, guid);
}

void Server::SendChunkContents(std::shared_ptr<Chunk> vc, NetworkGUID guid) {
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
		auto& b = blocks[i];
		if(!b.IsValid()) continue;

		auto bID = b.blockID;
		packetInfo[i] = bID << 2 | b.orientation;
	}

	// TODO: Compression should happen here
	// A large majority of chunks will be mostly empty

	Packet p;
	p.reliability = RELIABLE_ORDERED;

	u32 numPackets = 0;

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
		numPackets++;

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

		numPackets++;
		network->Send(p, guid);
	}

	// logger << "Sent " << numPackets << " packets";
}

void Server::SendNeighborhoodTransform(std::shared_ptr<ChunkNeighborhood> neigh, NetworkGUID guid) {
	Packet p;
	p.WriteType(PacketType::SetNeighborhoodTransform);
	p.Write<u16>(neigh->neighborhoodID);
	p.Write(neigh->position);
	p.Write(neigh->rotation);
	// TODO: Neighborhood velocities

	network->Send(p, guid);
}
