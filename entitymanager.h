#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <vector>
#include <queue>

#include "common.h"
#include "singleton.h"

// Handles updating
// Entity queries

// List/Pool of entities

struct Entity;
struct Component;

namespace Ogre {
	class SceneNode;
}

struct EntityManager : Singleton<EntityManager> {
	std::vector<Entity*> entities;
	std::queue<Component*> newComponents;
	u32 entityIdCounter;

	EntityManager();
	~EntityManager();

	// Entity* CreateEntity(Ogre::SceneNode*);
	// Entity* CreateEntity(const std::string& name, const vec3& pos = vec3::ZERO, const quat& = quat::IDENTITY);
	void DestroyEntity(Entity*);
	Entity* FindEntity(const std::string& name);

	// Update updates all active entites
	void Update();
	void LateUpdate();

	void DestroyAllEntities();
};

#endif