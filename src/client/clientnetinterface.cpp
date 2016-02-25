#include "clientnetinterface.h"
#include "playermanager.h"
#include "chunkmanager.h"
#include "netplayer.h"
#include "debugdraw.h"
#include "network.h"
#include "block.h"
#include "chunk.h"

static Log logger{"ClientNetInterface"};

// Messages from server
static void OnUpdatePlayerState(Packet&);

static void OnNewChunk(Packet&);
static void OnRemoveChunk(Packet&);

static void OnSetBlock(Packet&);
static void OnChunkDownload(Packet&);
static void OnSetChunkNeighborhood(Packet&);
static void OnSetNeighborhoodTransform(Packet&);

void ClientNetInterface::Update(std::shared_ptr<Network> net) {
	net->Update();

	Packet packet;
	while(net->GetPacket(&packet)) {
		u8 type = packet.ReadType();

		switch(type) {
			case PacketType::RemoteJoin: {
				u16 playerID;
				u8 joinType;
				packet.Read(playerID);
				packet.Read(joinType);

				auto pmgr = PlayerManager::Get();
				if(!pmgr->GetPlayer(playerID))
					pmgr->AddPlayer(std::make_shared<NetPlayer>(), playerID);

				if(!joinType)
					logger << "Player " << playerID << " joined";

			} break;

			case PacketType::RemoteLeave: {
				u16 playerID;
				u8 reason;
				packet.Read(playerID);
				packet.Read(reason);

				auto pmgr = PlayerManager::Get();
				pmgr->RemovePlayer(playerID);

				if(!reason)
					logger << "Player " << playerID << " disconnected";
				else
					logger << "Player " << playerID << " lost connection";
			} break;

			case PacketType::UpdatePlayerState: OnUpdatePlayerState(packet); break;

			case PacketType::NewChunk: OnNewChunk(packet); break;
			case PacketType::RemoveChunk: OnRemoveChunk(packet); break;

			case PacketType::SetBlock: OnSetBlock(packet); break;
			case PacketType::ChunkDownload: OnChunkDownload(packet); break;
			case PacketType::SetChunkNeighborhood: OnSetChunkNeighborhood(packet); break;
			case PacketType::SetNeighborhoodTransform: OnSetNeighborhoodTransform(packet); break;
		}
	}
}

void ClientNetInterface::UpdatePlayerState(vec3 p, vec3 v, quat o, quat e) {
	Packet packet;
	packet.WriteType(PacketType::UpdatePlayerState);
	// This is a fairly large packet, optimisation warranted 
	packet.Write(p);
	packet.Write(v);
	packet.Write(o);
	packet.Write(e);

	packet.reliability = UNRELIABLE_SEQUENCED;
	packet.priority = MEDIUM_PRIORITY;

	auto net = Network::Get();
	net->Send(packet);
}

void ClientNetInterface::SetPlayerName(const std::string&) {
	// TODO
}

void ClientNetInterface::SetPlayerTeam(u8) {
	// TODO
}

void ClientNetInterface::SetPlayerSector(u8) {
	// TODO
}

void ClientNetInterface::SetBlock(u16 chunkID, ivec3 pos, u16 type, u8 orientation) {
	Packet packet;
	packet.WriteType(PacketType::SetBlock);
	packet.Write(chunkID);
	packet.Write(pos);
	packet.Write<u16>(type << 2 | orientation);

	Network::Get()->Send(packet);
}

void ClientNetInterface::DoInteract(u16 chunkID, ivec3 pos) {
	Packet packet;
	packet.WriteType(PacketType::PlayerInteract);
	packet.Write(chunkID);
	packet.Write(pos);

	Network::Get()->Send(packet);
}

void ClientNetInterface::RequestRefreshChunks() {
	Packet packet;
	packet.WriteType(PacketType::ChunkDownload);

	Network::Get()->Send(packet);
}

/*
	                                                                                                             
	 ad88888ba                                                          88b           d88                        
	d8"     "8b                                                         888b         d888                        
	Y8,                                                                 88`8b       d8'88                        
	`Y8aaaaa,    ,adPPYba, 8b,dPPYba, 8b       d8  ,adPPYba, 8b,dPPYba, 88 `8b     d8' 88 ,adPPYba,  ,adPPYb,d8  
	  `"""""8b, a8P_____88 88P'   "Y8 `8b     d8' a8P_____88 88P'   "Y8 88  `8b   d8'  88 I8[    "" a8"    `Y88  
	        `8b 8PP""""""" 88          `8b   d8'  8PP""""""" 88         88   `8b d8'   88  `"Y8ba,  8b       88  
	Y8a     a8P "8b,   ,aa 88           `8b,d8'   "8b,   ,aa 88         88    `888'    88 aa    ]8I "8a,   ,d88  
	 "Y88888P"   `"Ybbd8"' 88             "8"      `"Ybbd8"' 88         88     `8'     88 `"YbbdP"'  `"YbbdP"Y8  
	                                                                                                 aa,    ,88  
	                                                                                                  "Y8bbdP"   
*/
void OnUpdatePlayerState(Packet& packet) {
	vec3 pos, vel;
	quat ori, eye;
	u16 playerID;

	packet.Read<u16>(playerID);
	packet.Read(pos);
	packet.Read(vel);
	packet.Read(ori);
	packet.Read(eye);

	auto pmgr = PlayerManager::Get();
	auto player = pmgr->GetPlayer(playerID);
	if(!player) return;

	player->SetPosition(pos);
	player->SetVelocity(vel);
	player->SetOrientation(ori);
	player->SetEyeOrientation(eye);
}

void OnNewChunk(Packet& packet) {
	auto chmgr = ChunkManager::Get();
	u16 chunkID;
	u16 neighborhoodID;
	u8 w,h,d;
	vec3 position;
	quat rotation;
	ivec3 poi;

	packet.Read(chunkID);
	packet.Read(neighborhoodID);
	packet.Read(w);
	packet.Read(h);
	packet.Read(d);

	auto ch = chmgr->CreateChunk(w,h,d);
	ch->chunkID = chunkID;

	if(!neighborhoodID) {
		packet.Read(position);
		packet.Read(rotation);

		ch->position = position;
		ch->rotation = rotation;

	}else{
		packet.Read(poi);

		auto neigh = chmgr->GetNeighborhood(neighborhoodID);
		if(!neigh) {
			neigh = chmgr->CreateNeighborhood();
			neigh->neighborhoodID = neighborhoodID;
			neigh->chunkSize = ivec3{w,h,d};
		}
		
		ch->SetNeighborhood(neigh);
		ch->positionInNeighborhood = poi;
		neigh->UpdateChunkTransform(ch);
	}

	// logger << "New chunk " << chunkID << " at " << position;
}

void OnRemoveChunk(Packet& packet) {
	auto chmgr = ChunkManager::Get();

	u16 chunkID;
	packet.Read(chunkID);

	chmgr->DestroyChunk(chunkID);
}

void OnSetBlock(Packet& packet) {
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
		else logger << "Block create failed at " << vxPos;
	}else{
		ch->DestroyBlock(vxPos);
	}
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

void OnSetChunkNeighborhood(Packet& packet) {
	u16 chunkID, neighborhoodID;
	packet.Read(chunkID);
	packet.Read(neighborhoodID);

	auto chmgr = ChunkManager::Get();
	auto ch = chmgr->GetChunk(chunkID);
	if(!ch) {
		logger << "Server tried to set neighborhood of unknown chunk!";
		return;
	}

	auto neigh = chmgr->GetNeighborhood(neighborhoodID);
	if(!neigh) {
		neigh = chmgr->CreateNeighborhood();
		neigh->neighborhoodID = neighborhoodID;
		neigh->chunkSize = ivec3{ch->width, ch->height, ch->depth};
	}

	ch->SetNeighborhood(neigh);
	packet.Read<ivec3>(ch->positionInNeighborhood);

	logger << ch->positionInNeighborhood;
	neigh->UpdateChunkTransform(ch);
}

void OnSetNeighborhoodTransform(Packet& packet) {
	u16 neighID;
	vec3 pos;
	quat rot;

	packet.Read(neighID);
	packet.Read(pos);
	packet.Read(rot);

	auto chmgr = ChunkManager::Get();
	auto neigh = chmgr->GetNeighborhood(neighID);
	if(!neigh) {
		logger << "Server tried to update transform of non-existent neighborhood";
		return;
	}

	neigh->position = pos;
	neigh->rotation = rot;
	neigh->UpdateChunkTransforms();
}

