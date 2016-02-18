#include "network.h"

#include <raknet/RakPeerInterface.h>
#include <raknet/MessageIdentifiers.h>

static Log logger{"Network"};

std::shared_ptr<Network> Network::Get() {
	static std::weak_ptr<Network> wp;
	std::shared_ptr<Network> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<Network>());

	return p;
}

void Network::Init() {
	peer = RakNet::RakPeerInterface::GetInstance();
	isConnected = false;
	isHosting = false;
}

void Network::Shutdown() {
	if(peer) peer->Shutdown(100);
}


void Network::Host(u16 port, u8 numConnections) {
	isHosting = true;
	isConnected = true;

	RakNet::SocketDescriptor sd{port, 0};
	peer->Startup(numConnections, &sd, 1);
	peer->SetMaximumIncomingConnections(numConnections);
	peer->SetOccasionalPing(true);
}

void Network::Connect(std::string address, u16 port) {
	isHosting = false;

	RakNet::SocketDescriptor sd{};
	peer->Startup(1, &sd, 1);
	peer->Connect(address.data(), port, nullptr, 0);
}

#include <map>

std::map<u32, std::string> packetNames {
	{ID_NO_FREE_INCOMING_CONNECTIONS, "ID_NO_FREE_INCOMING_CONNECTIONS"},
	{ID_CONNECTION_REQUEST_ACCEPTED, "ID_CONNECTION_REQUEST_ACCEPTED"},
	{ID_DISCONNECTION_NOTIFICATION, "ID_DISCONNECTION_NOTIFICATION"},
	{ID_NEW_INCOMING_CONNECTION, "ID_NEW_INCOMING_CONNECTION"},
	{ID_CONNECTION_LOST, "ID_CONNECTION_LOST"},

	{ID_USER_PACKET_ENUM, "ID_USER_PACKET_ENUM"},
	{ID_USER_PACKET_ENUM+1, "ID_USER_PACKET_ENUM + 1"},
	{ID_USER_PACKET_ENUM+2, "ID_USER_PACKET_ENUM + 2"},
};

void Network::Update() {
	for(auto packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive()) {
		auto type = packet->data[0];
		// logger << "Packet " << (isHosting?"srv ":"cli ") << packet->length << " " << packetNames[type];

		// TODO: This could be a bit more sophisticated
		switch(type) {
			case ID_CONNECTION_REQUEST_ACCEPTED: isConnected = true; break;
			case ID_CONNECTION_LOST: isConnected = !isHosting; break;
		}

		packets.emplace(packet->data, packet->length, packet->guid);
	}
}

void Network::Send(const Packet& p, NetworkGUID to) {
	if(!isConnected && !isHosting) throw "Tried to send while not connected";

	bool broadcast = (to == RakNet::UNASSIGNED_RAKNET_GUID);
	peer->Send(&p.bitstream, p.priority, p.reliability, 0, to, broadcast);
}

void Network::Broadcast(const Packet& p, NetworkGUID excl) {
	if(!isHosting) throw "Can't broadcast while not hosting";

	peer->Send(&p.bitstream, p.priority, p.reliability, 0, excl, true);
}

bool Network::GetPacket(Packet* p) {
	if(!p) throw "Null packet in GetPacket";
	if(packets.empty()) return false;

	auto& pk = packets.front();
	*p = std::move(pk);

	packets.pop();
	return true;
}


/*
	                                                               
	88888888ba                        88                           
	88      "8b                       88                    ,d     
	88      ,8P                       88                    88     
	88aaaaaa8P' ,adPPYYba,  ,adPPYba, 88   ,d8  ,adPPYba, MM88MMM  
	88""""""'   ""     `Y8 a8"     "" 88 ,a8"  a8P_____88   88     
	88          ,adPPPPP88 8b         8888[    8PP"""""""   88     
	88          88,    ,88 "8a,   ,aa 88`"Yba, "8b,   ,aa   88,    
	88          `"8bbdP"Y8  `"Ybbd8"' 88   `Y8a `"Ybbd8"'   "Y888  
	                                                               
	                                                               
*/
using RakNet::RakNetGUID;

Packet::Packet() {}
Packet::Packet(Packet&& o) : fromGUID{o.fromGUID} { bitstream.Write(o.bitstream); }
Packet::Packet(u8* data, u32 len, RakNetGUID from) : bitstream{data, len, true}, fromGUID{from} {}

Packet& Packet::operator= (Packet&& o) {
	bitstream.Reset();
	bitstream.Write(o.bitstream);
	fromGUID = o.fromGUID;
	return *this;
}

void Packet::Reset() {
	bitstream.Reset();
}

void Packet::WriteType(u8 id) {
	bitstream.Write((RakNet::MessageID)id);
}

void Packet::Write(quat q) {
	bitstream.WriteNormQuat(q.w, q.x, q.y, q.z);
}


u8 Packet::ReadType() {
	u8 type;
	Read(type);
	return type;
}

void Packet::Read(quat& q) {
	bitstream.ReadNormQuat(q.w, q.x, q.y, q.z);
}
