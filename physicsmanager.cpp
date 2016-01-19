#include "entitymotionstate.h"
#include "physicsmanager.h"
#include "bullethelpers.h"
// #include "meshinfo.h"
// #include "apptime.h"
#include "entity.h"
// #include "app.h"

#include <algorithm>
#include <limits>

// THIS METHOD SUCKS
template<>
PhysicsManager* Singleton<PhysicsManager>::instance = nullptr;

// void LayerNearCollisionFilterCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& dispatchInfo) {
// 	auto proxy0 = collisionPair.m_pProxy0;
// 	auto proxy1 = collisionPair.m_pProxy1;

// 	auto ud0 = static_cast<btCollisionObject*>(proxy0->m_clientObject)->getUserPointer();
// 	auto ud1 = static_cast<btCollisionObject*>(proxy1->m_clientObject)->getUserPointer();

// 	auto comp0 = static_cast<Component*>(ud0);
// 	auto comp1 = static_cast<Component*>(ud1);

// 	auto col0 = comp0->As<ColliderComponent>(false);
// 	auto col1 = comp1->As<ColliderComponent>(false);

// 	if(!col0 || !col1) {
// 		std::cout << "One of the components in LayerNearCollisionFilterCallback isn't a collider" << std::endl;
// 		return;
// 	}

// 	auto ecg = PhysicsManager::GetSingleton()->enabledCollisionGroups;
// 	auto mask = (col0->collisionGroups & col1->collisionGroups & ecg);

// 	if(mask == 0) return;

// 	// Continue physics stuff as usual
// 	dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
// }

PhysicsManager::PhysicsManager(f32 refreshRate)
	: timestep{1.f/refreshRate} {

	auto collisionConfig = new btDefaultCollisionConfiguration{};
	broadphase = new Broadphase{};
	dispatcher = new Dispatcher{collisionConfig};
	solver = new Solver{};

	world = new World{dispatcher, broadphase, solver, collisionConfig};
	world->setGravity({0, -30., 0});
	// dispatcher->setNearCallback(LayerNearCollisionFilterCallback);
}

PhysicsManager::~PhysicsManager(){
	if(!world) return;
	delete world;
	delete solver;
	delete dispatcher->getCollisionConfiguration();
	delete dispatcher;
	delete broadphase;
}

void PhysicsManager::Update(f32 dt){
	// world->stepSimulation((btScalar)(AppTime::deltaTime * AppTime::phystimescale), 10);
	world->stepSimulation((btScalar)dt, 10);
	currentStamp++;

	// Remove all deleted and null trigger collisions
	// This happens before processing because collisions
	//	can be nullified by NotifyColliderRemoval
	for(auto& acp: activeColliderPairs){
		if(!acp) continue;
		if(!acp->collider0 || !acp->collider1){
			delete acp;
			acp = nullptr;
		}
	}

	auto begin = activeColliderPairs.begin();
	auto end = activeColliderPairs.end();
	activeColliderPairs.erase(std::remove(begin, end, nullptr), end);

	// Find all collisions between colliders and collider triggers
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
		auto obA = (btCollisionObject*)contactManifold->getBody0();
		auto obB = (btCollisionObject*)contactManifold->getBody1();

		int numContacts = contactManifold->getNumContacts();
		for (int j = 0; j < numContacts; j++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() < 0.f) {
				auto ud0 = obA->getUserPointer();
				auto ud1 = obB->getUserPointer();

				auto col0 = static_cast<Component*>(ud0)->As<ColliderComponent>(false);
				auto col1 = static_cast<Component*>(ud1)->As<ColliderComponent>(false);

				// If both objects are valid colliders
				if(!col0 || !col1) continue;

				// Process collision
				if(col0->trigger || col1->trigger) {
					ProcessTriggerCollision(col0, col1);
				}else{
					ProcessCollision(col0, col1);
				}
			}
		}
	}

	// Remove stale trigger collisions and notify relevant
	//	entities
	for(auto& cp: activeColliderPairs){
		if(!cp) continue;
		if(!cp->collider0 || !cp->collider1) continue;

		// Should work fine given unsigned integer underflow
		if((currentStamp - cp->stamp) > 1){
			if(cp->collider0->trigger
			|| cp->collider1->trigger){
				cp->collider0->entity->OnTriggerLeave(cp->collider1);
				cp->collider1->entity->OnTriggerLeave(cp->collider0);
			}else{
				cp->collider0->entity->OnCollisionLeave(cp->collider1);
				cp->collider1->entity->OnCollisionLeave(cp->collider0);
			}

			// Setting these to nullptr flags them for cleanup
			cp->collider0 = nullptr;
			cp->collider1 = nullptr;
		}
	}
}

struct RaycastCallback : btCollisionWorld::ClosestRayResultCallback {
	RaycastCallback(const btVector3& start, const btVector3& end, u32 _collisionMask = ~0u, s32 _layer = 0)
		: ClosestRayResultCallback{start, end}, collisionMask{_collisionMask}, layer{_layer} {}

	u32 collisionMask;
	s32 layer;

	bool needsCollision(btBroadphaseProxy* proxy0) const override {
		auto co = (ColliderComponent::RigidBody*)proxy0->m_clientObject;
		auto col = (ColliderComponent*)co->getUserPointer();

		auto ecg = PhysicsManager::GetSingleton()->enabledCollisionGroups;
		auto mask = (col->collisionGroups & collisionMask & ecg);

		return mask;
	}
};

auto PhysicsManager::Raycast(const vec3& start, const vec3& dir, u32 collisionMask) -> RaycastResult {
	return Linecast(start, start + dir, collisionMask);
}

auto PhysicsManager::Linecast(const vec3& ostart, const vec3& oend, u32 collisionMask) -> RaycastResult {
	auto start = o2bt(ostart);
	auto end = o2bt(oend);

	RaycastCallback rayCallback{start, end, collisionMask};

	// Perform raycast
	world->rayTest(start, end, rayCallback);

	if(rayCallback.hasHit()) {
		end = rayCallback.m_hitPointWorld;
		auto normal = rayCallback.m_hitNormalWorld;
		auto col = (ColliderComponent*)rayCallback.m_collisionObject->getUserPointer();

		return {col, bt2o(end), bt2o(normal), rayCallback.m_closestHitFraction};
	}

	return {nullptr, vec3{0.f}, vec3{0.f}, std::numeric_limits<f32>::infinity()};
}

void PhysicsManager::ProcessCollision(ColliderComponent* col0, ColliderComponent* col1){
	// Test if this collision is already being tracked
	auto begin = activeColliderPairs.begin();
	auto end = activeColliderPairs.end();
	auto it = std::find_if(begin, end,
		[col0, col1](const ColliderPair* cp){
			return (cp->collider0 == col0 && cp->collider1 == col1)
				|| (cp->collider0 == col1 && cp->collider1 == col0);
		});

	// If it already exists, update stamp and stop
	if(it != end) {
		(*it)->stamp = currentStamp;
		return;
	}

	// Otherwise register new pair
	auto acp = new ColliderPair{col0, col1, currentStamp};
	activeColliderPairs.push_back(acp);

	// Notify relevant entities
	col0->entity->OnCollisionEnter(col1);
	col1->entity->OnCollisionEnter(col0);
}

void PhysicsManager::ProcessTriggerCollision(ColliderComponent* col0, ColliderComponent* col1){
	ColliderComponent* trigger = nullptr;
	ColliderComponent* col = nullptr;

	// Figure out which is the trigger and which is the collider
	if(col0->trigger){
		trigger = col0;
		col = col1;
	}

	if(col1->trigger){
		trigger = col1;
		col = col0;
	}

	// Give up if neither are triggers
	if(!trigger) return;

	// Test if this collision is already being tracked
	auto begin = activeColliderPairs.begin();
	auto end = activeColliderPairs.end();
	auto it = std::find_if(begin, end,
		[trigger, col](const ColliderPair* cp){
			return (cp->collider0 == trigger && cp->collider1 == col)
				|| (cp->collider0 == col && cp->collider1 == trigger);
		});

	// If it already exists, update stamp and stop
	if(it != end) {
		(*it)->stamp = currentStamp;
		return;
	}

	// Otherwise register new pair
	auto acp = new ColliderPair{trigger, col, currentStamp};
	activeColliderPairs.push_back(acp);

	// Notify relevant entities
	trigger->entity->OnTriggerEnter(col);
	col->entity->OnTriggerEnter(trigger);
}

void PhysicsManager::NotifyColliderRemoval(ColliderComponent* c){
	if(!c) return;

	for(auto& cp: activeColliderPairs){
		if(!cp) continue;
		if(!cp->collider0 || !cp->collider1) continue;
		if(cp->collider0 != c && cp->collider1 != c) continue;

		// Should work fine given unsigned integer underflow
		if(cp->collider0->entity
		&& cp->collider1->entity) {
			if(cp->collider0->trigger
			|| cp->collider1->trigger){
				cp->collider0->entity->OnTriggerLeave(cp->collider1);
				cp->collider1->entity->OnTriggerLeave(cp->collider0);
			}else{
				cp->collider0->entity->OnCollisionLeave(cp->collider1);
				cp->collider1->entity->OnCollisionLeave(cp->collider0);
			}
		}

		// Setting these to nullptr flags them for cleanup
		cp->collider0 = nullptr;
		cp->collider1 = nullptr;
	}
}

/*
	  ,ad8888ba,              88 88 88          88
	 d8"'    `"8b             88 88 ""          88
	d8'                       88 88             88
	88             ,adPPYba,  88 88 88  ,adPPYb,88  ,adPPYba, 8b,dPPYba,
	88            a8"     "8a 88 88 88 a8"    `Y88 a8P_____88 88P'   "Y8
	Y8,           8b       d8 88 88 88 8b       88 8PP""""""" 88
	 Y8a.    .a8P "8a,   ,a8" 88 88 88 "8a,   ,d88 "8b,   ,aa 88
	  `"Y8888Y"'   `"YbbdP"'  88 88 88  `"8bbdP"Y8  `"Ybbd8"' 88
*/

void ColliderComponent::OnInit() {
	auto world = PhysicsManager::GetSingleton()->world;

	CreateCollider();
	motionState = new EntityMotionState(entity);

	btScalar mass = 0.;
	btVector3 inertia {0,0,0};
	if(dynamic) {
		mass = 10.;
		collider->calculateLocalInertia(mass, inertia);
	}

	RigidBodyInfo bodyInfo{mass, motionState, collider, inertia};
	body = new RigidBody{bodyInfo};
	body->setUserPointer(this);

	world->addRigidBody(body);
}

void ColliderComponent::OnDestroy() {
	auto physman = PhysicsManager::GetSingleton();

	if(physman)
		physman->world->removeRigidBody(body);
	delete motionState;
	delete body;
	delete collider;

	if(physman)
		physman->NotifyColliderRemoval(this);
}

void ColliderComponent::DisableRotation(){
	body->setAngularFactor(0.f);
}

void ColliderComponent::SetTrigger(bool _trigger){
	// http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Callbacks_and_Triggers
	// TODO: lookup ghost objects

	trigger = _trigger;

	auto flags = body->getCollisionFlags();
	if(trigger){
		flags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
	}else{
		flags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
	}

	body->setCollisionFlags(flags);
}

void ColliderComponent::SetKinematic(bool _kine, bool readd){
	kinematic = _kine;

	auto physman = PhysicsManager::GetSingleton();
	if(readd) physman->world->removeRigidBody(body);

	auto flags = body->getCollisionFlags();
	if(kinematic){
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
	}else{
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
	}
	body->setCollisionFlags(flags);

	if(readd) physman->world->addRigidBody(body);
}

void ColliderComponent::SetAutosleep(bool as){
	body->setActivationState(as?WANTS_DEACTIVATION:DISABLE_DEACTIVATION);
}

void ColliderComponent::SetFriction(f32 val) {
	body->setFriction(val);
}


void ColliderComponent::Wakeup(){
	body->activate();
}

vec3 ColliderComponent::GetVelocity() const {
	return bt2o(body->getLinearVelocity());
}
void ColliderComponent::SetVelocity(const vec3& v){
	body->setLinearVelocity(o2bt(v));
}

vec3 ColliderComponent::GetPosition() const {
	return bt2o(body->getWorldTransform().getOrigin());
}
void ColliderComponent::SetPosition(const vec3& p) {
	auto world = body->getWorldTransform();
	world.setOrigin(o2bt(p));
	body->setWorldTransform(world);
}

quat ColliderComponent::GetOrientation() const {
	return bt2o(body->getWorldTransform().getRotation());
}

void ColliderComponent::SetOrientation(const quat& o) {
	auto world = body->getWorldTransform();
	world.setRotation(o2bt(o));
	body->setWorldTransform(world);
}


/*
	  ,ad8888ba,                                           88
	 d8"'    `"8b                                    ,d    ""
	d8'                                              88
	88            8b,dPPYba,  ,adPPYba, ,adPPYYba, MM88MMM 88  ,adPPYba,  8b,dPPYba,
	88            88P'   "Y8 a8P_____88 ""     `Y8   88    88 a8"     "8a 88P'   `"8a
	Y8,           88         8PP""""""" ,adPPPPP88   88    88 8b       d8 88       88
	 Y8a.    .a8P 88         "8b,   ,aa 88,    ,88   88,   88 "8a,   ,a8" 88       88
	  `"Y8888Y"'  88          `"Ybbd8"' `"8bbdP"Y8   "Y888 88  `"YbbdP"'  88       88
*/
void BoxColliderComponent::CreateCollider() {
	auto hs = dimensions/2.f;
	collider = new btBoxShape{o2bt(hs)};
}

void CapsuleColliderComponent::CreateCollider() {
	auto radius = dimensions.x/2.f;
	auto height = dimensions.y - dimensions.x; // Because the hemisphere caps aren't included
	collider = new btCapsuleShape{radius, height};
}

void ConeColliderComponent::CreateCollider() {
	auto radius = dimensions.x/2.f;
	auto height = dimensions.y;
	collider = new btConeShape{radius, height};
}

void CylinderColliderComponent::CreateCollider() {
	auto radius = dimensions.x/2.f;
	auto height = dimensions.y;
	collider = new btConeShape{radius, height};
}

void SphereColliderComponent::CreateCollider() {
	collider = new btSphereShape{dimensions.x/2.f};
}

void MeshColliderComponent::CreateCollider() {
	auto trimesh = new btTriangleMesh();

	// Yuck
	// for(u32 i = 0; i < entity->ogreEntity->getMesh()->getNumSubMeshes(); i++){
	// 	auto sm = entity->ogreEntity->getMesh()->getSubMesh(i);
	// 	auto smVertices = GetOgreSubMeshVerticesFlat(sm);

	// 	for(u32 i = 0; i < smVertices.size()/3; i++) {
	// 		btVector3 vs[] = {
	// 			o2bt(smVertices[i*3+0]),
	// 			o2bt(smVertices[i*3+1]),
	// 			o2bt(smVertices[i*3+2]),
	// 		};

	// 		trimesh->addTriangle(vs[0], vs[1], vs[2]);
	// 	}
	// }

	collider = new btBvhTriangleMeshShape(trimesh, !dynamic /* Optimise for static */);
}

void ConvexHullColliderComponent::CreateCollider() {
	auto hull = new btConvexHullShape();

	// Yuck
	// for(u32 i = 0; i < entity->ogreEntity->getMesh()->getNumSubMeshes(); i++){
	// 	auto sm = entity->ogreEntity->getMesh()->getSubMesh(i);
	// 	auto smVertices = GetOgreSubMeshVertices(sm);

	// 	for(u32 j = 0; j < smVertices.size(); j++) {
	// 		hull->addPoint(o2bt(smVertices[j]));
	// 	}
	// }

	collider = hull;
}