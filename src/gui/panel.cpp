#include "gui/panel.h"
#include "gui.h"

void PanelElement::Update() {
	auto gui = GUI::Get();
	auto b = &gui->builder;

	auto m = GetMetrics();
	b->AddQuad(m);

	// auto bl = vec2{m->bottomLeft};
	// auto tr = vec2{m->topRight};
	// auto br = vec2{tr.x, bl.y};
	// auto tl = vec2{bl.x, tr.y};

	// b->depth = m->depth;
	// b->Add(m->bottomLeft, vec2{0, 0});
	// b->Add(tl, vec2{0, 1});
	// b->Add(m->topRight, vec2{1, 1});
	// b->Add(br, vec2{1, 0});
}
