#ifndef SERVERPLAYER_H
#define SERVERPLAYER_H

#include "common.h"
#include "playerbase.h"

struct ServerPlayer : PlayerBase {
	quat orientation;
	vec3 position;
	u8 sector;
};


#endif