#ifndef PLAYER_H
#define PLAYER_H

#include "shared/common.h"
#include "shared/physics.h"
#include "shared/playerbase.h"

struct Camera;

// TODO: Focus/interaction states

struct Player : PlayerBase {
	static constexpr f32 Height = 1.5f;

	Collider* collider;
	RigidBody* rigidbody;
	std::shared_ptr<Camera> camera;

	vec2 camRot {0};
	u16 blockType {1};
	u8 blockRot {0};

	bool noclip;

	Player(std::shared_ptr<Camera>);
	~Player();

	void Update() override;

	bool IsNoclip() override { return noclip; }
	void SetNoclip(bool) override;

	bool IsLocal() override { return true; }
};

#endif