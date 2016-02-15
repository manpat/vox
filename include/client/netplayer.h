#ifndef NETPLAYER_H
#define NETPLAYER_H

#include "common.h"
#include "playerbase.h"

struct NetPlayer : PlayerBase {
	quat orientation;
	vec3 position;
	u8 sector;

	void Render() override;

	void SetPosition(vec3) override;
	void SetVelocity(vec3) override;
	void SetOrientation(quat) override;
};


#endif