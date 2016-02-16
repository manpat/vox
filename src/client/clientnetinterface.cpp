#include "clientnetinterface.h"
#include "playermanager.h"
#include "chunkmanager.h"
#include "voxelchunk.h"
#include "netplayer.h"
#include "debugdraw.h"
#include "network.h"
#include "block.h"

static Log logger{"ClientNetInterface"};

void ClientNetInterface::Update(std::shared_ptr<Network> net) {
	net->Update();

	Packet packet;
	while(net->GetPacket(&packet)) {
		u8 type = packet.ReadType();

		switch(type) {
			case PacketType::UpdatePlayerState: {
				vec3 pos, vel;
				quat ori;
				u16 playerID;

				packet.Read<u16>(playerID);
				packet.Read(pos);
				packet.Read(vel);
				packet.Read(ori);

				auto pmgr = PlayerManager::Get();
				auto player = pmgr->GetPlayer(playerID);
				if(!player) {
					player = std::make_shared<NetPlayer>();
					pmgr->AddPlayer(player, playerID);
				}

				player->SetPosition(pos);
				player->SetVelocity(vel);
				player->SetOrientation(ori);
			} break;

			case PacketType::RemoteJoin: {
				u16 playerID;
				packet.Read<u16>(playerID);

				auto pmgr = PlayerManager::Get();
				pmgr->AddPlayer(std::make_shared<NetPlayer>(), playerID);
			} break;

			case PacketType::RemoteLeave: {
				u16 playerID;
				packet.Read<u16>(playerID);

				auto pmgr = PlayerManager::Get();
				pmgr->RemovePlayer(playerID);
			} break;

			case PacketType::NewChunk: {
				auto chmgr = ChunkManager::Get();
				u16 chunkID;
				u16 neighborhoodID;
				u8 w,h,d;
				vec3 position;

				packet.Read(chunkID);
				packet.Read(neighborhoodID);
				packet.Read(w);
				packet.Read(h);
				packet.Read(d);
				packet.Read(position);

				auto neigh = chmgr->GetNeighborhood(neighborhoodID);
				if(!neigh) {
					neigh = chmgr->CreateNeighborhood();
					neigh->neighborhoodID = neighborhoodID;
					neigh->chunkSize = ivec3{w,h,d};
				}

				auto ch = chmgr->CreateChunk(w,h,d);
				ch->SetNeighborhood(neigh);
				// ch->modelMatrix = glm::translate(position);
				ch->position = position;
				ch->chunkID = chunkID;

				logger << "New chunk " << chunkID;
			} break;

			case PacketType::RemoveChunk: {
				auto chmgr = ChunkManager::Get();

				u16 chunkID;
				packet.Read(chunkID);

				chmgr->DestroyChunk(chunkID);
			} break;

			case PacketType::SetBlock: {
				auto chmgr = ChunkManager::Get();
				u8 orientation;
				u16 chunkID, blockType;
				ivec3 vxPos;

				// Assume vxPos is in bounds
				packet.Read(chunkID);
				packet.Read<ivec3>(vxPos);
				packet.Read(blockType);
				packet.Read(orientation);

				auto ch = chmgr->GetChunk(chunkID);
				if(!ch) {
					logger << "Missing chunkID " << chunkID;
					return;
				}

				if(blockType) {
					auto blk = ch->CreateBlock(vxPos, blockType);
					if(blk) blk->orientation = orientation;
					else logger << "Block create failed";
				}else{
					ch->DestroyBlock(vxPos);
				}
			} break;
		}
	}
}

void ClientNetInterface::UpdatePlayerState(vec3 p, vec3 v, quat o) {
	Packet packet;
	packet.WriteType(PacketType::UpdatePlayerState);
	// This is a fairly large packet, optimisation warranted 
	packet.Write<vec3>(p);
	packet.Write<vec3>(v);
	packet.Write(o);

	auto net = Network::Get();
	net->Send(packet);
}

void ClientNetInterface::SetPlayerName(const std::string&) {

}

void ClientNetInterface::SetPlayerTeam(u8) {

}

void ClientNetInterface::SetPlayerSector(u8) {

}

void ClientNetInterface::SetBlock(u16 chunkID, ivec3 pos, u16 type, u8 orientation) {
	Packet packet;
	packet.WriteType(PacketType::SetBlock);
	packet.Write(chunkID);
	packet.Write(pos);
	packet.Write(type);
	packet.Write(orientation);

	auto net = Network::Get();
	net->Send(packet);
}

