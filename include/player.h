#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include "physics.h"

struct Camera;

struct PlayerBase {
	virtual void Update() {}

	virtual vec3 GetPosition() { return vec3{0}; }
	virtual vec3 GetVelocity() { return vec3{0}; }
	virtual quat GetOrientation() { return quat{0,0,0,1}; }

	virtual vec3 GetEyePosition() { return vec3{0}; }
	virtual quat GetEyeOrientation() { return quat{0,0,0,1}; }

	virtual void SetPosition(vec3) {}
	virtual void SetVelocity(vec3) {}
	virtual void SetOrientation(quat) {}

	// Eye position should be derived from body position
	virtual void SetEyeOrientation(quat) {}

	virtual bool IsNoclip() { return false; }
	virtual void SetNoclip(bool) {}

	virtual bool IsLocal() { return false; }

	// Is in GUI?

	// Get gravity volume/direction
	// Set gravity volume

	// Get current tool/held item
};

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