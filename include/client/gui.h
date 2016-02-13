#ifndef GUI_H
#define GUI_H

#include "common.h"
#include "gui/builder.h"

// http://www.gamedev.net/page/resources/_/technical/game-programming/creating-a-very-simple-gui-system-for-small-games-part-i-r3652

struct Camera;
struct Element;

struct GUI {
	static std::shared_ptr<GUI> Get();

	std::vector<std::weak_ptr<Element>> elements;
	std::shared_ptr<Camera> camera;

	std::weak_ptr<Element> selectedElement;
	std::weak_ptr<Element> hoveredElement;

	u32 screenWidth, screenHeight;
	f32 aspect;

	vec2 cellSize;
	vec2 gridSize;

	GUIBuilder builder;
	u32 texture;

	void Init();
	void Update();
	void Render();

	std::shared_ptr<Element> GetElementAt(vec2);

	void InjectMouseMove(vec2);
	void InjectMouseButton(bool); // Left button only for now

	template<class T, class... A>
	std::shared_ptr<T> CreateElement(A&&... a);

	void AddElement(std::shared_ptr<Element>);
};

template<class T, class... A>
std::shared_ptr<T> GUI::CreateElement(A&&... a) {
	auto el = std::make_shared<T>(std::forward<A>(a)...);
	AddElement(el);
	return el;
}

#endif