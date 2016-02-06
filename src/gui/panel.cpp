#include "gui/panel.h"
#include "gui.h"

void PanelElement::Update() {
	auto gui = GUI::Get();
	auto b = &gui->builder;
	auto m = GetMetrics();

	b->depth = m->depth;
	b->Add9Slice(m->bottomLeft, m->topRight, panelSlice, vec2{16,16}, 2.f);
}
