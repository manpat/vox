#include "clientnetinterface.h"
#include "network.h"

void ClientNetInterface::Update(std::shared_ptr<Network> net) {
	net->Update();

	Packet packet;
	while(net->GetPacket(&packet)) {
		u8 type = packet.ReadType();

		if(type == PacketType::UpdatePlayerState) {
			vec3 pos;
			packet.Read(pos);
			Debug::Point(pos);
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

void ClientNetInterface::SetBlock(u16 chunkID, ivec3 pos, u16 type) {

}

