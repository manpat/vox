#include "gui/button.h"
#include "gui.h"

const vec2 buttonBasePos {160,0};
const vec2 buttonSize {16,16};

void ButtonElement::Update() {
	auto gui = GUI::Get();
	auto b = &gui->builder;
	auto m = GetMetrics();

	b->depth = m->depth;
	b->Add9Slice(m->bottomLeft, m->topRight, slice, buttonSize, 3.f);
}

void ButtonElement::OnMouseEnter() {
	slice = buttonBasePos + vec2{buttonSize.x,0};
}

void ButtonElement::OnMouseLeave() {
	slice = buttonBasePos;
}

static Log logger{"ButtonElement"};

void ButtonElement::OnMouseDown() {
	logger << "OnMouseDown";
	slice = buttonBasePos + vec2{buttonSize.x*2,0};
}

void ButtonElement::OnMouseUp() {
	logger << "OnMouseUp";
	slice = buttonBasePos + vec2{buttonSize.x,0};
}

void ButtonElement::OnClick() {
	logger << "Click";
}
