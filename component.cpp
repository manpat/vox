#include "component.h"

u32 Component::componentIdCounter = 0;

bool Component::SameType(Component* c) const {
	return c && (c->typeHash == typeHash);
}