#ifndef GUI_ELEMENT_H
#define GUI_ELEMENT_H

#include "common.h"

struct Element {
	struct CalculatedMetrics {
		// In absolute coordinates
		vec2 bottomLeft;
		vec2 topRight;
	};

	std::weak_ptr<Element> self;
	std::weak_ptr<Element> parent;
	std::vector<std::shared_ptr<Element>> children;

	CalculatedMetrics calculatedMetrics;

	// Coordinates are on 12x12 grid
	vec2 position; // Relative
	vec2 offset; // Absolute
	vec2 proportions; // Relative
	vec2 size; // Absolute

	u8 origin : 4; // xxyy
	u8 dirty : 1;
	u8 active : 1;

	CalculatedMetrics* GetMetrics();

	Element();

	template<class T, class... A>
	std::shared_ptr<T> CreateChild(A&&... a);

	void AddChild(std::shared_ptr<Element>);
	void FlagDirty();
	void SetOrigin(s8 x, s8 y);

	void ConcreteRender();
	void ConcreteUpdate();

	virtual void Update() {};
	virtual void Render() {};
};

template<class T, class... A>
std::shared_ptr<T> Element::CreateChild(A&&... a) {
	auto el = std::make_shared<T>(std::forward<A>(a)...);
	el->self = el;
	AddChild(el);
	return el;
}

#endif