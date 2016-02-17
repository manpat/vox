#include "clientnetinterface.h"
#include "playermanager.h"
#include "chunkmanager.h"
#include "voxelchunk.h"
#include "netplayer.h"
#include "debugdraw.h"
#include "network.h"
#include "block.h"

static Log logger{"ClientNetInterface"};

static void OnChunkDownload(Packet&);

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

				orientation = blockType & 3;
				blockType >>= 2;

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

			case PacketType::ChunkDownload: OnChunkDownload(packet); break;
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
	packet.Write<u16>(type << 2 | orientation);

	auto net = Network::Get();
	net->Send(packet);
}


void OnChunkDownload(Packet& p) {
	u16 chunkID, offset;
	u8 size;

	p.Read(chunkID);
	p.Read(offset);
	p.Read(size);

	auto chmgr = ChunkManager::Get();
	auto ch = chmgr->GetChunk(chunkID);
	if(!ch) {
		logger << "Downloading chunk that doesn't exist";
		return;
	}

	u8 w = ch->width;
	u8 h = ch->height;
	u8 d = ch->depth;

	u8 x, y, z;
	z = offset % d;
	y = (offset / d) % h;
	x = (offset / d / h) % w;

	for(u8 i = 0; i < size; i++) {
		u16 blockType;
		p.Read(blockType);

		u8 orientation = blockType&3;
		blockType >>= 2;

		auto blk = ch->CreateBlock(ivec3{x,y,z}, blockType);
		if(blk) blk->orientation = orientation;

		if(++z >= d) {
			if(++y >= h) {
				++x; // No check because it should never happen
				y = 0;
			}
			z = 0;
		}
	}
}
