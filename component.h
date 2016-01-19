#ifndef COMPONENT_H
#define COMPONENT_H

#include <string>
#include <typeinfo>

#include "common.h"
#include "typehelpers.h"
// #include "opaquetype.h"

// Is an interface
// Updatable
// Can recieve messages

// Note: Cannot be pooled easily because it relies on polymorphism

struct Entity;
struct ColliderComponent;

struct Component {
	static u32 componentIdCounter;

	// This is the owning entity
	Entity* entity = nullptr;
	// Unique identifier, id 0 is invalid
	u32 id = 0;
	// This is for fast type comparisons
	size_t typeHash = 0;
	// This is for debugging
	std::string typeName = "Component";
	// This determines whether OnUpdate is triggered
	bool enabled = true;

	template<class C>
	Component(C*) : id(++componentIdCounter), typeHash(typeid(C).hash_code()), typeName(getTypeName<C>()) {}
	virtual ~Component() {}

	// OnInit is called after the component has been initialised and attached to
	//	an entity
	virtual void OnInit() {};

	// OnAwake is called before the before the first update after initialisation
	virtual void OnAwake() {};

	// OnRemove is called after the owning entity calls RemoveComponent
	virtual void OnRemove() {};

	// OnDestroy is called before the component is destroyed
	virtual void OnDestroy() {};

	// OnUpdate is called once per frame if the component is enabled
	virtual void OnUpdate() {};

	// OnLateUpdate is called once per frame if the component is enabled, and after
	//	audio and physics managers have updated
	virtual void OnLateUpdate() {};

	// OnLayerChange is called when the parent entity's layer changes
	virtual void OnLayerChange() {};

	// These are called for the relevant physics events
	virtual void OnCollisionEnter(ColliderComponent*) {};
	virtual void OnCollisionLeave(ColliderComponent*) {};
	virtual void OnTriggerEnter(ColliderComponent*) {};
	virtual void OnTriggerLeave(ColliderComponent*) {};

	// OnMessage is called when SendMessage is called on the owning entity
	// virtual void OnMessage(const std::string&, const OpaqueType&) {}; // TODO

	// Checks derived component type
	template<class C>
	bool IsType() const { return typeid(C).hash_code() == typeHash; }

	// Tests whether this is the same type as another component
	bool SameType(Component*) const;

	// Upcast to derived component
	template<class C>
	const C* As(bool fatal = true) const {
		if(!IsType<C>()) {
			if(!fatal) return nullptr;
			else throw std::string("Component cast error: As<") + getTypeName<C>() + "> {"+ typeName +"}";
		}
		return static_cast<const C*>(this);
	}

	// Upcast to derived component
	template<class C>
	C* As(bool fatal = true) {
		if(!IsType<C>()) {
			if(!fatal) return nullptr;
			else throw std::string("Component cast error: As<") + getTypeName<C>() + "> {"+ typeName +"}";
		}
		return static_cast<C*>(this);
	}
};

#endif