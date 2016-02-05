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
		if(el->active) el->Update();

		el->GetMetrics(); // Temp
	}
}

#include "debugdraw.h"

void GUI::Render() {
	// TEMP
	vec3 bl {0};
	vec3 tr {gridSize*cellSize, 0};
	vec3 br {tr.x, bl.y, 0};
	vec3 tl {bl.x, tr.y, 0};
	vec3 gridCol {.3};
	f32 z = -.1f;

	for(f32 y = 0; y < tr.y; y+=cellSize.y)
		Debug::Line(vec3{0,y,z}, vec3{12.f,y,z}, gridCol);

	for(f32 x = 0; x < tr.x; x+=cellSize.x)
		Debug::Line(vec3{x,0,z}, vec3{x,12.f*cellSize.y,z}, gridCol);

	Debug::Line(bl,tl, vec3{0,0,1});
	Debug::Line(bl,br, vec3{0,0,1});
	Debug::Line(tr,tl, vec3{0,0,1});
	Debug::Line(tr,br, vec3{0,0,1});
	// TEMP

	for(auto& wel: elements) {
		if(auto el = wel.lock())
			if(el->active) el->Render();
	}
}

void GUI::AddElement(std::weak_ptr<Element> e) {
	elements.emplace_back(e);
}