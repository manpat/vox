#ifndef GUI_H
#define GUI_H

#include "common.h"

// http://www.gamedev.net/page/resources/_/technical/game-programming/creating-a-very-simple-gui-system-for-small-games-part-i-r3652

struct Camera;
struct Element;

struct GUI {
	static std::shared_ptr<GUI> Get();

	std::vector<std::weak_ptr<Element>> elements;

	u32 screenWidth, screenHeight;
	f32 aspect;

	vec2 cellSize;
	vec2 gridSize;

	std::shared_ptr<Camera> camera;

	void Init();
	void Update();
	void Render();

	void AddElement(std::weak_ptr<Element>);
};

#endif