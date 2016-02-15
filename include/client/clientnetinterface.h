#ifndef CLIENTNETINTERFACE_H
#define CLIENTNETINTERFACE_H

#include "common.h"

struct Network;

struct ClientNetInterface {
	static void Update(std::shared_ptr<Network>);

	static void UpdatePlayerState(vec3,vec3,quat);
	static void SetPlayerName(const std::string&);
	static void SetPlayerTeam(u8);
	static void SetPlayerSector(u8);
	static void SetBlock(u16 chunkID, ivec3 pos, u16 type);
};

#endif