#include "netplayer.h"
#include "debugdraw.h"

NetPlayer::NetPlayer() : realPosition{0.f}, position{0.f}, velocity{0.f} {}

void NetPlayer::Update() {
	// Basic interpolation
	position = glm::mix(
		position, 
		realPosition + velocity * timeSinceUpdate * .5f, 
		Time::dt * 10.f + timeSinceUpdate);

	eyeOrientation = glm::mix(eyeOrientation, realEyeOri, Time::dt * 5.f);
	orientation = glm::mix(orientation, realOri, Time::dt * 5.f);

	timeSinceUpdate += Time::dt;
}

void NetPlayer::Render() {
	auto eye = GetEyePosition();

	Debug::Point(position);
	Debug::Point(eye);
	Debug::Line(position, eye);
	Debug::Line(eye, eye + vec3{0,0,-1} * eyeOrientation, vec3{0,0,1});
}

void NetPlayer::SetPosition(vec3 p) {
	timeSinceUpdate = 0.f;
	realPosition = p;
}

void NetPlayer::SetVelocity(vec3 v) {
	velocity = v;
}

void NetPlayer::SetOrientation(quat o) {
	realOri = o;
}

void NetPlayer::SetEyeOrientation(quat o) {
	realEyeOri = o;
}

vec3 NetPlayer::GetEyePosition() {
	return position + orientation * vec3{0,PlayerHeight,0};
}

quat NetPlayer::GetEyeOrientation() {
	return eyeOrientation;
}