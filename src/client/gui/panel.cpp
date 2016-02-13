#include "gui/panel.h"
#include "gui.h"

void PanelElement::Update() {
	auto gui = GUI::Get();
	auto b = &gui->builder;
	auto m = GetMetrics();

	panelSlice = vec2{128,0};
	auto size = vec2{32,32};
	// auto size = vec2{16,16};

	b->depth = m->depth;
	b->Add9Slice(m->bottomLeft, m->topRight, panelSlice, size, 3.f);
}
