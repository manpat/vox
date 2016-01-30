#include "physics.h"

World* Physics::world = nullptr;
Solver* Physics::solver = nullptr;
Broadphase* Physics::broadphase = nullptr;
Dispatcher* Physics::dispatcher = nullptr;

void Physics::Init() {
	auto collisionConfig = new btDefaultCollisionConfiguration{};
	broadphase = new Broadphase{};
	dispatcher = new Dispatcher{collisionConfig};
	solver = new Solver{};

	world = new World{dispatcher, broadphase, solver, collisionConfig};
	world->setGravity({0, -30., 0});
}

auto Physics::Raycast(vec3 _begin, vec3 _end) -> RaycastResult {
	auto beg = o2bt(_begin);
	auto end = o2bt(_end);

	btCollisionWorld::ClosestRayResultCallback rayCallback{beg, end};
	Physics::world->rayTest(beg, end, rayCallback);

	RaycastResult res;
	res.hit = rayCallback.hasHit();
	res.position = bt2o(rayCallback.m_hitPointWorld);
	res.normal = bt2o(rayCallback.m_hitNormalWorld);
	res.rigidbody = (RigidBody*) rayCallback.m_collisionObject;
	return res;
}

