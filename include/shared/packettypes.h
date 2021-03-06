#ifndef PACKETTYPES_H
#define PACKETTYPES_H

#include "common.h"
#include <raknet/MessageIdentifiers.h>

// Client to server doesn't send playerID
// Server to client does

namespace PacketType {
	enum {
		// [S->C] Notify players that another has joined
		// PlayerID, u8 type/reason
		RemoteJoin = ID_USER_PACKET_ENUM,
		RemoteLeave,

		// [S<>C] Encodes position, velocity, orientation, eyeOrientation
		UpdatePlayerState,

		// [S<>C] Notify of changes to low-freq player state
		// If number of packets become an issue, these could be combined
		//	into a single packet with a subtype
		SetPlayerName,
		SetPlayerTeam,
		SetPlayerSector,

		// [S<>C] Notify of a single block change
		// If vxPosition is out of bounds, the block is created in the appropriate
		//	position in a neighboring chunk
		// ChunkID, vx position, blockID:14, orientation:2
		SetBlock,

		// [S->C] Notify of chunk stuff
		// ChunkID, NeighborhoodID, size(u8,u8,u8), [positionInNeighborhood | {position, rotation} if neigh == 0]
		NewChunk,
		// ChunkID
		RemoveChunk,
		
		// [S->C] Set position and rotation of neighborhood
		// NeighborhoodID, position, rotation
		SetNeighborhoodTransform,

		// [S->C] Set the neighborhood of a chunk
		// ChunkID, NeighborhoodID, Position in neighborhood
		SetChunkNeighborhood,

		// [S<>C] Set some portion of a chunk
		// S->C ChunkID, u16 offset, u8 numBlocks, {blockID:14, orientation:2}...
		// S<-C nothing
		// Limit 245 blocks per packet
		// Assumes chunk size will never exceed 32x32x32
		ChunkDownload,

		// [S<-C] Inform server of block interaction
		// ChunkID, x:5, y:5, z:5
		PlayerInteract,
	};
}

#endif