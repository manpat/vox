#ifndef GUI_PANEL_H
#define GUI_PANEL_H

#include "common.h"
#include "gui/element.h"

struct PanelElement : Element {
	vec2 panelSlice = vec2{0,0};

	void Update() override;
};

#endif