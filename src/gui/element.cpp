#include "gui.h"
#include "gui/element.h"

static Log logger{"Element"};

auto Element::GetMetrics() -> CalculatedElementMetrics* {
	if(dirty) {
		auto gui = GUI::Get();
		auto p = parent.lock();

		vec2 absSize;
		vec2 absPos;

		if(p) {
			auto pm = p->GetMetrics();
			vec2 psize = pm->topRight - pm->bottomLeft;
			vec2 cellSize = psize / gui->gridSize;

			absSize = proportions * cellSize + size * gui->cellSize;
			absPos = position * cellSize + offset * gui->cellSize + pm->bottomLeft;
			calculatedMetrics.depth = depth + pm->depth + 0.1f;

		}else{ // Root node
			absSize = (proportions + size) * gui->cellSize;
			absPos = (position + offset) * gui->cellSize;
			calculatedMetrics.depth = depth;
		}

		auto ox = (origin&12) >> 2;
		auto oy = origin&3;

		if(ox == 1) absPos.x -= absSize.x / 2.f;
		else if(ox == 2) absPos.x -= absSize.x;

		// This means the internal origin will be bottom left
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
	active = true;
	origin = 0; // bottom left
}

void Element::ConcreteUpdate() {
	Update();
	for(auto& el: children) {
		el->ConcreteUpdate();
	}
}

void Element::ConcreteRender() {
	Render();
	for(auto& el: children) {
		el->ConcreteRender();
	}
}

std::shared_ptr<Element> Element::TestPoint(vec2 p) {
	auto m = GetMetrics();

	auto blchk = m->bottomLeft.x > p.x || m->bottomLeft.y > p.y;
	auto trchk = m->topRight.x < p.x || m->topRight.y < p.y;

	if(blchk || trchk) return nullptr;

	return self.lock();
}

void Element::FlagDirty() {
	dirty = true;
	for(auto& el: children) {
		el->FlagDirty();
	}
}

void Element::AddChild(std::shared_ptr<Element> el) {
	children.emplace_back(el);
	el->parent = self;
	el->FlagDirty();
}

void Element::SetOrigin(s8 x, s8 y) {
	x++; y++;
	origin = ((x<<2)&12) | (y & 3);
	dirty = true;
}