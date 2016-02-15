#ifndef LOCALPLAYER_H
#define LOCALPLAYER_H

#include "shared/common.h"
#include "shared/physics.h"
#include "shared/playerbase.h"

struct Camera;

// TODO: Focus/interaction states
// TODO: Fill in base methods

struct LocalPlayer : PlayerBase {
	static constexpr f32 Height = 1.5f;

	Collider* collider;
	RigidBody* rigidbody;
	std::shared_ptr<Camera> camera;

	vec2 camRot {0};
	u16 blockType {1};
	u8 blockRot {0};

	bool noclip;

	LocalPlayer(std::shared_ptr<Camera>);
	~LocalPlayer();

	void Update() override;

	bool IsNoclip() override { return noclip; }
	void SetNoclip(bool) override;

	bool IsLocal() override { return true; }
};

#endif