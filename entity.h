#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <unordered_map>

#include "common.h"

// Collection of Components
// Collection of Children
// Userdata
// Parent
// Id

// Updatable
// Transform operations (modifications to ogre transform)
// Component Queries/Operations
// Message Distribution

struct Component;
struct FramePool;
struct PhysicsManager;
struct ColliderComponent;

struct Entity {
	friend PhysicsManager;
	static FramePool* messagePool;

	// Vector of components, owned by this entity
	std::vector<Component*> components;

	// Vector of children entities, owned by entity manager
	std::vector<Entity*> children;

	// Parent entity, null if root entity
	Entity* parent;

	// Collider pointer for caching purposes, since most entities
	//	will have one. It assumes each entity will only ever have
	//	one collider
	ColliderComponent* collider;

	// Unique identifier, id 0 is invalid
	//	id 0 can be used for pooling
	u32 id;

	// The layer than this entity exists in
	//	NOTE: setting directly will do nothing
	s32 layer;

	// Whether or not this should recieve updates
	bool enabled;

	// Init resets Entity to default state and cleans up previous data.
	// 	To help with pooling
	void Init();

	// Destroy cleans up components
	void Destroy();

	// Destroy cleans up components and children
	void DestroyRecurse();

	// Update updates all attached and active components
	void Update();

	// LateUpdate late updates all attached and active components
	void LateUpdate();

	// AddChild attaches a child entity
	void AddChild(Entity*);
	// RemoveChild detaches a child entity but does not destroy it
	void RemoveChild(Entity*);
	// DestroyChild detaches and destroys a child entity
	void DestroyChild(Entity*);
	// OrphanSelf detaches self from parent
	void OrphanSelf();

	// AddComponent<C,A...> constructs a component with type C with arguments
	//	of types A... or void. Returns new component
	template<class C, class... A>
	C* AddComponent(A...);
	// AddComponent attaches an already constructed component
	void AddComponent(Component*);
	// RemoveComponent removes a component if it is attached but does not destroy it
	void RemoveComponent(Component*);
	// DestroyComponent removes a component and deletes it if it is attached
	void DestroyComponent(Component*);

	// FindComponent returns the first component of type C or nullptr
	template<class C>
	C* FindComponent();

	// SendMessage broadcasts a message to attached components
	// If zero arguments are given, a nullptr packet is sent to OnMessage
	// If one argument is given, a copy of that argument is sent to OnMessage
	// If more than one argument is given, a std::tuple of all arguments is sent to OnMessage
	//
	// For example: entity->SendMessage("messageType", 1, 2, 3.f, 'a'); sends the
	//	packet std::tuple<s32, s32, f32, char>
	template<class... A>
	void SendMessage(const std::string&, A... arguments);
	void SendMessage(const std::string&);

	// SendMessageRecurse broadcasts a message to attached components
	//	and child entities
	// See SendMessage
	template<class... A>
	void SendMessageRecurse(const std::string&, A...);

protected:
	void OnCollisionEnter(ColliderComponent*);
	void OnCollisionLeave(ColliderComponent*);
	void OnTriggerEnter(ColliderComponent*);
	void OnTriggerLeave(ColliderComponent*);
};

#include "entity.inl"

#endif