#include "gui.h"
#include "gui/element.h"

#include "camera.h"

static Log logger{"GUI"};

std::shared_ptr<GUI> GUI::Get() {
	static std::weak_ptr<GUI> wp;
	std::shared_ptr<GUI> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<GUI>());

	return p;
}

void GUI::Init() {
	aspect = (f32)screenWidth/screenHeight;

	cellSize = vec2{1.f, 1.f/aspect};
	gridSize = vec2{12.f, 12.f};
	vec2 absSize = cellSize*gridSize;

	auto proj = glm::ortho<f32>(0.f, absSize.x, 0, absSize.y, -1.f, 1.f);
	camera = std::make_shared<Camera>(proj);
}

void GUI::Update() {
	auto end = elements.end();
	elements.erase(std::remove_if(elements.begin(), end, 
		[](const auto& p) {return p.expired();}), end);
	
	for(auto& wel: elements) {
		auto el = wel.lock();
		if(el->active) el->ConcreteUpdate();
	}
}

void GUI::Render() {
	for(auto& wel: elements) {
		if(auto el = wel.lock())
			if(el->active) el->ConcreteRender();
	}
}

void GUI::AddElement(std::shared_ptr<Element> e) {
	elements.emplace_back(e);
	e->self = e;
}