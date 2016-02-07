#include "gui/button.h"
#include "gui.h"

void ButtonElement::Update() {
	auto gui = GUI::Get();
	auto b = &gui->builder;
	auto m = GetMetrics();

	b->depth = m->depth;
	b->Add9Slice(m->bottomLeft, m->topRight, slice, vec2{16,16}, 2.f);
}

void ButtonElement::OnMouseEnter() {
	slice = vec2{16*3,0};
}

void ButtonElement::OnMouseLeave() {
	slice = vec2{16*2,0};
}

static Log logger{"ButtonElement"};

void ButtonElement::OnMouseDown() {
	logger << "OnMouseDown";
	slice = vec2{16*4,0};
}

void ButtonElement::OnMouseUp() {
	logger << "OnMouseUp";
	slice = vec2{16*3,0};
}

void ButtonElement::OnClick() {
	logger << "Click";
}
