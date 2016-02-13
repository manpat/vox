#ifndef PHYSICS_H
#define PHYSICS_H

#include <btBulletDynamicsCommon.h>
#include "bullethelpers.h"

using Broadphase = btDbvtBroadphase;
using Dispatcher = btCollisionDispatcher;
using Solver = btSequentialImpulseConstraintSolver;
using World = btDiscreteDynamicsWorld;
using RigidBody = btRigidBody;
using RigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo;
using Collider = btCollisionShape;

// TODO: Single collider representation
// TODO: Multiple worlds for ships
struct Physics {
	struct RaycastResult {
		RigidBody* rigidbody;
		vec3 position;
		vec3 normal;
		bool hit;
	};

	static World* world;
	static Solver* solver;
	static Broadphase* broadphase;
	static Dispatcher* dispatcher;

	static void Init();
	static RaycastResult Raycast(vec3 begin, vec3 end);
};

#endif