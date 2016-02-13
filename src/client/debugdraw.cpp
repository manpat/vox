#include "shaderregistry.h"
#include "debugdraw.h"
#include "camera.h"
#include "gui.h"

std::vector<Debug::LineCmd> Debug::lineCmds {};
std::vector<Debug::PointCmd> Debug::pointCmds {};
std::vector<Debug::LineCmd> Debug::uiLineCmds {};
std::vector<Debug::PointCmd> Debug::uiPointCmds {};
std::shared_ptr<ShaderProgram> Debug::debugProgram {nullptr};

u32 Debug::vbo = 0;
u32 Debug::uivbo = 0;

void Debug::Init() {
	debugProgram = ShaderRegistry::CreateProgram("_debug", "shaders/debug.vs", "shaders/debug.fs");

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &uivbo);
}

void Debug::Render() {
	auto c = Camera::mainCamera.lock();
	auto gc = GUI::Get()->camera;

	glBindBuffer(GL_ARRAY_BUFFER, uivbo);
	glBufferData(GL_ARRAY_BUFFER, (uiPointCmds.size() + uiLineCmds.size()*2) * sizeof(Debug::PointCmd), nullptr, GL_DYNAMIC_DRAW);
	if(uiLineCmds.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, 
			0, 
			uiLineCmds.size()*sizeof(Debug::LineCmd), &uiLineCmds[0]);

	if(uiPointCmds.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, 
			uiLineCmds.size()*sizeof(Debug::LineCmd), 
			uiPointCmds.size()*sizeof(Debug::PointCmd), &uiPointCmds[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, (pointCmds.size() + lineCmds.size()*2) * sizeof(Debug::PointCmd), nullptr, GL_DYNAMIC_DRAW);
	if(lineCmds.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, 
			0, 
			lineCmds.size()*sizeof(Debug::LineCmd), &lineCmds[0]);

	if(pointCmds.size() > 0)
		glBufferSubData(GL_ARRAY_BUFFER, 
			lineCmds.size()*sizeof(Debug::LineCmd), 
			pointCmds.size()*sizeof(Debug::PointCmd), &pointCmds[0]);

	glClear(GL_DEPTH_BUFFER_BIT);
	debugProgram->Use();

	glEnableVertexAttribArray(8);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(PointCmd), nullptr);
	glVertexAttribPointer(8, 3, GL_FLOAT, false, sizeof(PointCmd), (void*)sizeof(vec3));

	c->SetUniforms(debugProgram.get());

	glEnable(GL_PROGRAM_POINT_SIZE);
	glLineWidth(5);

	if(lineCmds.size() > 0)
		glDrawArrays(GL_LINES, 0, lineCmds.size()*2);

	if(pointCmds.size() > 0)
		glDrawArrays(GL_POINTS, lineCmds.size()*2, pointCmds.size());

	glDisable(GL_PROGRAM_POINT_SIZE);

	glBindBuffer(GL_ARRAY_BUFFER, uivbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(PointCmd), nullptr);
	glVertexAttribPointer(8, 3, GL_FLOAT, false, sizeof(PointCmd), (void*)sizeof(vec3));

	gc->SetUniforms(debugProgram.get());
	glPointSize(3);
	glLineWidth(3);

	if(uiLineCmds.size() > 0)
		glDrawArrays(GL_LINES, 0, uiLineCmds.size()*2);

	if(uiPointCmds.size() > 0)
		glDrawArrays(GL_POINTS, uiLineCmds.size()*2, uiPointCmds.size());
	
	glDisableVertexAttribArray(8);

	lineCmds.clear();
	pointCmds.clear();

	uiLineCmds.clear();
	uiPointCmds.clear();
}

void Debug::Point(f32 x, f32 y, f32 z, vec3 color) {
	Point(vec3{x,y,z}, color);
}
void Debug::Point(vec3 pos, vec3 color) {
	pointCmds.push_back({pos, color});
}

void Debug::PointUI(f32 x, f32 y, vec3 color) {
	PointUI(vec2{x,y}, color);
}
void Debug::PointUI(vec2 pos, vec3 color) {
	uiPointCmds.push_back({vec3{pos, 0}, color});
}

void Debug::Line(vec3 beg, vec3 end, vec3 color) {
	Line(beg, end, color, color);
}
void Debug::Line(vec3 beg, vec3 end, vec3 bcol, vec3 ecol) {
	lineCmds.push_back({
		{beg, bcol},
		{end, ecol}
	});
}

void Debug::LineUI(vec2 beg, vec2 end, vec3 color) {
	LineUI(beg, end, color, color);
}
void Debug::LineUI(vec2 beg, vec2 end, vec3 bcol, vec3 ecol) {
	uiLineCmds.push_back({
		{vec3{beg, 0}, bcol},
		{vec3{end, 0}, ecol}
	});
}

