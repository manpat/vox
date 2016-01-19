#include "entitymanager.h"
#include "component.h"
#include "entity.h"
#include "pool.h"
// #include "app.h"

#include <algorithm>

template<>
EntityManager* Singleton<EntityManager>::instance = nullptr;

EntityManager::EntityManager(): entityIdCounter{0} {
	// 1MB per framebuffer, 1MB swap
	Entity::messagePool = new FramePool{1u<<20};
	instance = this;
}

EntityManager::~EntityManager(){
	DestroyAllEntities();

	// We don't need checks here because deleting a nullptr
	//	is a nop
	delete Entity::messagePool;
	Entity::messagePool = nullptr;
	instance = nullptr;
}

// Entity* EntityManager::CreateEntity(Ogre::SceneNode* sn){
// 	auto e = new Entity{};
// 	e->Init();
// 	e->id = ++entityIdCounter; // Smallest valid id is 1
// 	// e->ogreSceneNode = sn;
// 	entities.push_back(e);
// 	return e;
// }

// Entity* EntityManager::CreateEntity(const std::string& name, const vec3& pos, const quat& rot){
// 	auto app = App::GetSingleton();

// 	auto e = new Entity{};
// 	e->Init();
// 	e->id = ++entityIdCounter; // Smallest valid id is 1
// 	// e->ogreSceneNode = app->rootNode->createChildSceneNode(name, pos, rot);
// 	entities.push_back(e);
// 	return e;
// }

// TODO: Verify that this does what I think it does
void EntityManager::DestroyEntity(Entity* e){
	if(!e) return;

	auto end = entities.end();
	entities.erase(std::remove(entities.begin(), end, e), end);

	// Destroy self and all children
	e->DestroyRecurse();
	delete e;
}

Entity* EntityManager::FindEntity(const std::string& name){
	// auto entit = std::find_if(entities.begin(), entities.end(), [&name](const Entity* e){
	// 	return name == e->GetName();
	// });

	// if(entit == entities.end()) return nullptr;
	// return *entit;
	return nullptr;
}

void EntityManager::Update(){
	Entity::messagePool->Update();

	while (!newComponents.empty()) {
		auto c = newComponents.front();
		if(c) {
			c->OnAwake();
		}
		newComponents.pop();
	}

	for(auto it = entities.begin(); it != entities.end(); ++it){
		auto e = *it;

		// Don't bother checking active because it's guaranteed that
		//	all entities will be.
		// Pooling not implemented
		if(e->enabled /*&& e->id != 0*/){
			e->Update();
		}
	}
}

void EntityManager::LateUpdate(){
	for(auto it = entities.begin(); it != entities.end(); ++it){
		auto e = *it;

		// Don't bother checking active because it's guaranteed that
		//	all entities will be.
		// Pooling not implemented
		if(e->enabled /*&& e->id != 0*/){
			e->LateUpdate();
		}
	}
}

void EntityManager::DestroyAllEntities(){
	for (auto it = entities.begin(); it != entities.end();) {
		auto e = *it;

		if (!e) {
			++it;
			continue;
		}

		// Don't try to destroy children
		// std::cout << "Destroying " << e->GetName() << std::endl;
		e->Destroy();
		it = entities.erase(it);

		delete e;
		e = nullptr;
	}

	//entities.clear();
	assert(entities.empty());

	// For some reason, std::queue doesn't have a clear, this is equivalent
	// This avoids the case where the scene is destroyed before newComponents
	//	are processed, leading to OnAwake being called on null components
	newComponents = std::queue<Component*>{};

	entityIdCounter = 0;
	Component::componentIdCounter = 0;
}