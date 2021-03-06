#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "network.h"
#include "serverplayer.h"

#include <map>

struct ChunkNeighborhood;
struct PlayerManager;
struct ChunkManager;
struct Chunk;

struct Server {
	std::shared_ptr<PlayerManager> playerManager;
	std::shared_ptr<ChunkManager> chunkManager;
	std::shared_ptr<Network> network;
	std::map<NetworkGUID, u16> guidToPlayerID;
	u16 playerIDCount;
	u16 chunkIDCount;
	u16 neighborhoodIDCount;

	void Run();

	void OnPlayerConnect(NetworkGUID);
	void OnPlayerDisonnect(NetworkGUID);
	void OnPlayerLostConnection(NetworkGUID);
	void OnPlayerStateUpdate(Packet&);
	void OnSetBlock(Packet&);
	void OnInteract(Packet&);

	// If guid is Unassigned, these broadcast
	void SendNewChunk(std::shared_ptr<Chunk>, NetworkGUID = RakNet::UNASSIGNED_RAKNET_GUID);
	void SendChunkContents(std::shared_ptr<Chunk>, NetworkGUID = RakNet::UNASSIGNED_RAKNET_GUID);
	void SendSetNeighborhood(std::shared_ptr<Chunk>, NetworkGUID = RakNet::UNASSIGNED_RAKNET_GUID);

	void SendNeighborhoodTransform(std::shared_ptr<ChunkNeighborhood>, NetworkGUID = RakNet::UNASSIGNED_RAKNET_GUID);
};


#endif