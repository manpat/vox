#include "server.h"
#include "network.h"
#include "serverplayer.h"
#include "playermanager.h"

#include <chrono>
#include <thread>

static Log logger{"Server"};

using namespace std::chrono;

void Server::Run() {
	network = Network::Get();
	network->Init();
	network->Host(16660, 10);

	playerManager = std::make_shared<PlayerManager>();
	playerIDCount = 0;

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
			}
		}

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

	packet.Reset();
	packet.WriteType(PacketType::SetNetID);
	packet.Write(playerID);
	network->Send(packet, guid);
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
	np.Write(pos);
	np.Write(vel);
	np.Write(ori);
	network->Broadcast(np, p.fromGUID);
}

