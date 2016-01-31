#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include "physics.h"

struct Camera;

// TODO: Focus/interaction states
struct Player {
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

	void Update();

	void SetNoclip(bool);
};

#endif