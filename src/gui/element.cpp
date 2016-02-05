#include "gui.h"
#include "gui/element.h"

static Log logger{"Element"};

auto Element::GetMetrics() -> CalculatedMetrics* {
	if(dirty) {
		auto ox = (origin&12) >> 2;
		auto oy = origin&3;

		vec2 absSize;
		vec2 absPos;

		auto gui = GUI::Get();
		auto p = parent.lock();

		if(p) {
			auto pm = p->GetMetrics();
			vec2 psize = pm->topRight - pm->bottomLeft;
			vec2 cellSize = psize / gui->gridSize;

			absSize = proportions * cellSize + size * gui->cellSize;
			absPos = position * cellSize + offset * gui->cellSize + pm->bottomLeft;

		}else{ // Root node
			absSize = (proportions + size) * gui->cellSize;
			absPos = (position + offset) * gui->cellSize;
		}

		if(ox == 1) absPos.x -= absSize.x / 2.f;
		else if(ox == 2) absPos.x -= absSize.x;

		// This means the internal origin will be bottomLeft
		if(oy == 1) absPos.y -= absSize.y / 2.f;
		else if(oy == 2) absPos.y -= absSize.y;

		calculatedMetrics.bottomLeft = absPos;
		calculatedMetrics.topRight = absPos + absSize;

		dirty = false;
	}

	return &calculatedMetrics;
}

Element::Element() {
	dirty = true;
}

void Element::Render() {
	for(auto& el: children) {
		el->Render();
	}
}

void Element::AddChild(std::shared_ptr<Element> el) {
	children.emplace_back(el);
	el->parent = self;
}

void Element::SetOrigin(s8 x, s8 y) {
	x++; y++;
	origin = ((x<<2)&12) | (y & 3);
	dirty = true;
}