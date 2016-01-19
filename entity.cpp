#include "physicsmanager.h"
#include "entitymanager.h"
#include "component.h"
#include "entity.h"
#include "pool.h"
// #include "app.h"

#include <algorithm>

#ifdef SendMessage
#undef SendMessage // TODO: Fix properly on Windows
#endif

FramePool* Entity::messagePool = nullptr;

void Entity::Init(){
	components = {};
	children = {};
	parent = nullptr;
	//userdata = {}; // TODO: Fix properly in VS2013
	// id set by entity manager
	layer = 0;

	enabled = true;
}

void Entity::Destroy(){
	for (auto it = components.begin(); it != components.end();) {
		auto c = *it;

		if (!c) {
			++it;
			continue;
		}

		c->OnDestroy();
		it = components.erase(it);

		delete c;
		c = nullptr;
	}

	//components.clear();
	assert(components.empty());

	children.clear();
}

void Entity::DestroyRecurse(){
	for(auto it = components.begin(); it != components.end(); ++it){
		auto c = *it;
		c->OnDestroy();
		delete c;
	}
	components.clear();

	// TODO: TEST

	// This will recurse and destroy all leaf nodes first
	//	Note that circular references will kill this
	for(auto e = children.begin(); e != children.end(); ++e){
		EntityManager::GetSingleton()->DestroyEntity(*e);
	}
	children.clear();
}

void Entity::Update(){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnUpdate();
	}
}

void Entity::LateUpdate(){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnLateUpdate();
	}
}

void Entity::AddChild(Entity* e){
	if(!e) return;
	if(e->parent) throw "Tried to child an entity that already has a parent";

	children.push_back(e);
	e->parent = this;
}

void Entity::RemoveChild(Entity* e){
	if(!e) return;

	// Remove e from children
	auto end = children.end();
	auto it = std::remove(children.begin(), end, e);

	// If it wasn't a child, give up
	if(it == end) return;

	children.erase(it, end);

	// Reset parent if e is actually a child
	e->parent = nullptr;
}

void Entity::DestroyChild(Entity* e){
	if(!e) return;

	// Remove e from children
	auto end = children.end();
	auto it = std::remove(children.begin(), end, e);

	// If it wasn't a child, give up
	if(it == end) return;

	children.erase(it, end);

	// Destroy it
	EntityManager::GetSingleton()->DestroyEntity(e);
}

void Entity::OrphanSelf(){
	if(!parent) return;
	parent->RemoveChild(this);
}

void Entity::AddComponent(Component* c){
	if(!c) return;
	if(c->entity) throw "Tried to add a component already attached to another entity";

	if(c->IsType<ColliderComponent>()){
		collider = static_cast<ColliderComponent*>(c);
	}

	c->entity = this;
	c->enabled = true;
	c->OnInit();

	components.push_back(c);
	EntityManager::GetSingleton()->newComponents.push(c);
}

void Entity::RemoveComponent(Component* c){
	if(!c) return;

	auto end = components.end();
	auto it = std::remove(components.begin(), end, c);

	if(it == end) return;

	if(collider == c){
		collider = nullptr;
	}

	components.erase(it, end);
	c->OnRemove();
	c->entity = nullptr;
}

void Entity::DestroyComponent(Component* c){
	if(!c) return;

	RemoveComponent(c);
	c->OnDestroy();
	delete c;
}

void Entity::SendMessage(const std::string& type){
	// OpaqueType ot;

	// for(auto c = components.begin(); c != components.end(); ++c){
	// 	(*c)->OnMessage(type, ot);
	// }
}
// SendMessage with arguments defined in entity.inl


void Entity::OnCollisionEnter(ColliderComponent* oc){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnCollisionEnter(oc);
	}
}
void Entity::OnCollisionLeave(ColliderComponent* oc){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnCollisionLeave(oc);
	}
}
void Entity::OnTriggerEnter(ColliderComponent* oc){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnTriggerEnter(oc);
	}
}
void Entity::OnTriggerLeave(ColliderComponent* oc){
	for(auto c = components.begin(); c != components.end(); ++c){
		(*c)->OnTriggerLeave(oc);
	}
}