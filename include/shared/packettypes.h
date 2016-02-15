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

		// [S->C] Sent on connect so the player can identify itself
		// This may not be necessary
		SetNetID,

		// [S<>C] Encodes position, velocity, orientation
		UpdatePlayerState,

		// [S<>C] Notify of changes to low-freq player state
		SetPlayerName,
		SetPlayerTeam,
		SetPlayerSector,

		// [S<>C] Notify of a single block change
		SetBlock,

		// [S->C] Notify of chunk stuff
		NewChunk,
		RemoveChunk,
	};
}

#endif