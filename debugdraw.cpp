#include "shaderregistry.h"
#include "debugdraw.h"
#include "overlay.h"
#include "camera.h"

std::vector<Debug::LineCmd> Debug::lineCmds {};
std::vector<Debug::PointCmd> Debug::pointCmds {};
std::shared_ptr<ShaderProgram> Debug::debugProgram {nullptr};

u32 Debug::vbo = 0;

struct DebugDrawOverlay : Overlay {
	DebugDrawOverlay(std::shared_ptr<Camera> c) : Overlay{c} { priority = 0xffffff; } 
	void Render() override {
		Debug::Render(camera.get());
	}
};

void Debug::Init(OverlayManager* om) {
	debugProgram = ShaderRegistry::CreateProgram("_debug", "shaders/debug.vs", "shaders/debug.fs");

	glGenBuffers(1, &vbo);

	glPointSize(5);
	glLineWidth(5);
	glEnable(GL_PROGRAM_POINT_SIZE);

	if(om)
		om->overlays.push_back(std::make_shared<DebugDrawOverlay>(Camera::mainCamera.lock()));
}

void Debug::Render(Camera* c) {
	glClear(GL_DEPTH_BUFFER_BIT);
	debugProgram->Use();

	c->SetUniforms(debugProgram.get());

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

	glEnableVertexAttribArray(8);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(PointCmd), nullptr);
	glVertexAttribPointer(8, 3, GL_FLOAT, false, sizeof(PointCmd), (void*)sizeof(vec3));

	if(lineCmds.size() > 0)
		glDrawArrays(GL_LINES, 0, lineCmds.size()*2);

	if(pointCmds.size() > 0)
		glDrawArrays(GL_POINTS, lineCmds.size()*2, pointCmds.size());
	
	glDisableVertexAttribArray(8);

	lineCmds.clear();
	pointCmds.clear();
}

void Debug::Point(f32 x, f32 y, f32 z, vec3 color) {
	Point(vec3{x,y,z}, color);
}

void Debug::Point(vec3 pos, vec3 color) {
	pointCmds.push_back({pos, color});
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

