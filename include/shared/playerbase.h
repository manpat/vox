#ifndef PLAYERBASE_H
#define PLAYERBASE_H

#include "common.h"

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

	// Player ID
	// Admin?
	// Team

	// Chat?
};

#endif