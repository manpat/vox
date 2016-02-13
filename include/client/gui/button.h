#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "gui/element.h"

struct ButtonElement : Element {
	vec2 slice = vec2{32,0};

	void Update() override;

	void OnMouseEnter() override;
	void OnMouseLeave() override;

	void OnMouseDown() override;
	void OnMouseUp() override;

	void OnClick() override;
};

#endif