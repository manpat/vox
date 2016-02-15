#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"
#include "packettypes.h"
#include <raknet/BitStream.h>
#include <raknet/RakNetTypes.h>
#include <type_traits>
#include <queue>

//// Transport layer
// Manages connections
// Responsible for actually sending and receiving packets
// Parses packets, filters  low level stuff
// Passes high level packets to replication layer

using NetworkGUID = RakNet::RakNetGUID;

namespace RakNet {
	class RakPeerInterface;
}

struct Packet {
	RakNet::BitStream bitstream;
	NetworkGUID fromGUID;

	Packet();
	Packet(Packet&&);
	Packet(u8*,u32,NetworkGUID);

	Packet& operator= (Packet&&);

	void Reset();
	void WriteType(u8);
	u8 ReadType();

	template<class I>
	void Write(I);
	void Write(quat);

	template<class I>
	void Read(I&);
	void Read(quat&);
};

template<class I>
void Packet::Write(I i) {
	bitstream.Write<I>(i);
}

template<class I>
void Packet::Read(I& i) {
	bitstream.Read<I>(i);
}

struct Network {
	RakNet::RakPeerInterface* peer;
	std::queue<Packet> packets;
	bool isHosting;
	bool isConnected;

	static std::shared_ptr<Network> Get();

	void Init();
	void Shutdown();
	void Host(u16 port, u8 numConnections);
	void Connect(std::string address, u16 port);
	void Update();

	void Send(const Packet&, NetworkGUID to = RakNet::UNASSIGNED_RAKNET_GUID);
	void Broadcast(const Packet&, NetworkGUID exclude = RakNet::UNASSIGNED_RAKNET_GUID); // Only called by server

	bool GetPacket(Packet*);
};

#endif