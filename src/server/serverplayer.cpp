#include "serverplayer.h"

void ServerPlayer::SetPosition(vec3 v) {
	position = v;
}
void ServerPlayer::SetVelocity(vec3 v) {
	velocity = v;
}
void ServerPlayer::SetOrientation(quat v) {
	orientation = v;
}
void ServerPlayer::SetEyeOrientation(quat v) {
	eyeOrientation = v;
}

vec3 ServerPlayer::GetPosition() {
	return position;
}
vec3 ServerPlayer::GetVelocity() {
	return velocity;
}
quat ServerPlayer::GetOrientation() {
	return orientation;
}
quat ServerPlayer::GetEyeOrientation() {
	return eyeOrientation;
}

NetworkGUID ServerPlayer::GetGUID() {
	return guid;
}