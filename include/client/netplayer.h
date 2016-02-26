#ifndef NETPLAYER_H
#define NETPLAYER_H

#include "common.h"
#include "physics.h"
#include "playerbase.h"

struct NetPlayer : PlayerBase {
	quat orientation;
	quat eyeOrientation;
	quat realOri;
	quat realEyeOri;

	vec3 realPosition;
	vec3 position;
	vec3 velocity;
	u8 sector;

	f32 timeSinceUpdate;

	NetPlayer();

	void Update() override;
	void Render() override;

	void SetPosition(vec3) override;
	void SetVelocity(vec3) override;
	void SetOrientation(quat) override;
	void SetEyeOrientation(quat) override;

	vec3 GetEyePosition() override;
	quat GetEyeOrientation() override;
};


#endif