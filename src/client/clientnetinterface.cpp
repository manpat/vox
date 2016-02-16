#include "clientnetinterface.h"
#include "playermanager.h"
#include "chunkmanager.h"
#include "netplayer.h"
#include "debugdraw.h"
#include "network.h"

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

			} break;

			case PacketType::RemoveChunk: {
				auto chmgr = ChunkManager::Get();
				
			} break;

			case PacketType::SetBlock: {
				auto chmgr = ChunkManager::Get();
				
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

