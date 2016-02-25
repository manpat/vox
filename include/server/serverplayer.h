#ifndef SERVERPLAYER_H
#define SERVERPLAYER_H

#include "common.h"
#include "playerbase.h"

struct ServerPlayer : PlayerBase {
	NetworkGUID guid;
	quat orientation;
	quat eyeOrientation;
	vec3 position;
	vec3 velocity;
	u8 sector;

	void SetPosition(vec3) override;
	void SetVelocity(vec3) override;
	void SetOrientation(quat) override;
	void SetEyeOrientation(quat) override;

	vec3 GetPosition() override;
	vec3 GetVelocity() override;
	quat GetOrientation() override;
	quat GetEyeOrientation() override;
	
	NetworkGUID GetGUID() override;
};


#endif