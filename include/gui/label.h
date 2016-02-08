#ifndef GUI_LABEL_H
#define GUI_LABEL_H

#include "gui/element.h"

struct TextMesh;

struct LabelElement : Element {
	std::unique_ptr<TextMesh> textMesh;

	LabelElement();
	void Render() override;
};

#endif