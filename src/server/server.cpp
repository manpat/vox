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
	network = Network::Get();
	network->Init();
	network->Host(16660, 10);

	chunkManager = ChunkManager::Get();
	playerManager = PlayerManager::Get();
	playerIDCount = 0;

	constexpr s32 startPlaneSize = 1;
	auto startPlaneNeigh = chunkManager->CreateNeighborhood();

	for(s32 cx = -startPlaneSize; cx <= startPlaneSize; cx++)
	for(s32 cz = -startPlaneSize; cz <= startPlaneSize; cz++){
		auto chunk = chunkManager->CreateChunk(30,30,10);
		chunk->SetNeighborhood(startPlaneNeigh);
		chunk->positionInNeighborhood = ivec3{cx, cz, 0};

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(ivec3{x,y,0}, 1);
	}

	logger << "Init";

	Packet packet;

	while(true) {
		network->Update();

		while(network->GetPacket(&packet)) {
			u8 type = packet.ReadType();
			// logger << "Got packet " << (u32) type;

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

void Server::OnPlayerConnect(NetworkGUID guid) {
	u16 playerID = ++playerIDCount;
	guidToPlayerID[guid] = playerID;
	playerManager->AddPlayer(std::make_shared<ServerPlayer>(), playerID);

	logger << "Client " << playerID << " connected [" << NetworkGUID::ToUint32(guid) << "]";

	Packet packet;
	packet.WriteType(PacketType::RemoteJoin);
	packet.Write(playerID);
	network->Broadcast(packet, guid);

	// TODO: Send all the chunks
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
	p.Read(orientation);

	auto ch = chunkManager->GetChunk(chunkID);
	if(!ch) {
		logger << "Client tried to modify chunk that isn't known to server";
		return;
	}

	if(!ch->InBounds(vxPos)) {
		// Create neighbor
		// Notify clients

		logger << "!! Neighbor creation not implemented";
		return;
	}

	if(!blockType) {
		ch->DestroyBlock(vxPos);

	}else{
		auto block = ch->CreateBlock(vxPos, blockType);
		block->orientation = orientation;
	}

	// Packet needs to be copied because vxPos and chunkID can change
	Packet np;
	np.WriteType(PacketType::SetBlock);
	np.Write(chunkID);
	np.Write(vxPos);
	np.Write(blockType);
	np.Write(orientation);

	// Send to all including sender
	network->Broadcast(np);
}