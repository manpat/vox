#include "gui.h"
#include "camera.h"
#include "gui/label.h"
#include "textrendering.h"
#include "shaderregistry.h"

#include "debugdraw.h"

LabelElement::LabelElement() {
	textMesh = std::make_unique<TextMesh>(Font::defaultFont);
	// textMesh->SetText("123456789abcdef\n123456789abcdef\n123456789abcdef\n123456789abcdef");
	textMesh->SetText("Test");
	interactive = false;
}

static Log logger{"LabelElement"};

void LabelElement::Render() {
	auto m = GetMetrics();
	auto gui = GUI::Get();
	auto textProgram = ShaderRegistry::GetProgram("text");

	textProgram->Use();
	gui->camera->SetUniforms(textProgram.get());
	
	glUniform4f(textProgram->GetUniform("color"), 1,1,1, 1);

	auto size = m->topRight - m->bottomLeft;

	auto scalexy = size/textMesh->size;
	auto scale = std::min(scalexy.x, scalexy.y);
	auto tmbl = m->bottomLeft + vec2{0, textMesh->size.y * scalexy.y};

	Debug::PointUI(m->topRight);
	Debug::PointUI(m->bottomLeft);
	Debug::PointUI(m->bottomLeft + vec2{0, textMesh->size.y * scale}, vec3{1,1,0});

	Debug::PointUI(tmbl, vec3{1,0,0});
	Debug::PointUI(tmbl + textMesh->size * vec2{1,-1} * scale, vec3{0,1,0});
	Debug::PointUI(tmbl + textMesh->size * vec2{1,-1} * scale * .5f, vec3{0,0,1});

	textMesh->modelMatrix = 
		glm::translate(vec3{tmbl, 0}) *
		glm::scale(vec3{scale});

	textMesh->Render();
}