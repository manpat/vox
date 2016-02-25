#include "netplayer.h"
#include "debugdraw.h"

void NetPlayer::Render() {
	auto eye = GetEyePosition();

	Debug::Point(position);
	Debug::Point(eye);
	Debug::Line(position, eye);
	Debug::Line(eye, eye + vec3{0,0,-1} * eyeOrientation, vec3{0,0,1});
}

void NetPlayer::SetPosition(vec3 p) {
	position = p;
}

void NetPlayer::SetVelocity(vec3) {}

void NetPlayer::SetOrientation(quat o) {
	orientation = o;
}

void NetPlayer::SetEyeOrientation(quat o) {
	eyeOrientation = o;
}

vec3 NetPlayer::GetEyePosition() {
	return position + orientation * vec3{0,PlayerHeight,0};
}