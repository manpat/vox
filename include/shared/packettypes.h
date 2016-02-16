#ifndef PACKETTYPES_H
#define PACKETTYPES_H

#include "common.h"
#include <raknet/MessageIdentifiers.h>

// Client to server doesn't send playerID
// Server to client does

namespace PacketType {
	enum {
		// [S->C] Notify players that another has joined
		RemoteJoin = ID_USER_PACKET_ENUM,
		RemoteLeave,

		// [S<>C] Encodes position, velocity, orientation
		UpdatePlayerState,

		// [S<>C] Notify of changes to low-freq player state
		// If number of packets become an issue, these could be combined
		//	into a single packet with a subtype
		SetPlayerName,
		SetPlayerTeam,
		SetPlayerSector,

		// [S<>C] Notify of a single block change
		// ChunkID, vx position, blockID, orientation
		SetBlock,

		// [S->C] Notify of chunk stuff
		// ChunkID, NeighborhoodID, size, position
		NewChunk,

		// ChunkID
		RemoveChunk,
	};
}

#endif