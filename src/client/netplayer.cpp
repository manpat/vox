#include "netplayer.h"
#include "debugdraw.h"

void NetPlayer::Render() {
	Debug::Point(position);
}

void NetPlayer::SetPosition(vec3 p) {
	position = p;
}

void NetPlayer::SetVelocity(vec3) {}

void NetPlayer::SetOrientation(quat o) {
	orientation = o;
}