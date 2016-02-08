#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

#include "common.h"
#include <vector>

struct ShaderProgram;

struct Debug {
	struct PointCmd {
		vec3 pos;
		vec3 color;
	};

	struct LineCmd {
		PointCmd begin, end;
	};

	static std::shared_ptr<ShaderProgram> debugProgram;
	static std::vector<PointCmd> pointCmds;
	static std::vector<LineCmd> lineCmds;
	static std::vector<PointCmd> uiPointCmds;
	static std::vector<LineCmd> uiLineCmds;
	static u32 vbo, uivbo;

	static void Init();
	static void Render();

	static void Point(f32 x, f32 y, f32 z, vec3 color = vec3{1});
	static void Point(vec3 pos, vec3 color = vec3{1});

	static void PointUI(f32 x, f32 y, vec3 color = vec3{1});
	static void PointUI(vec2 pos, vec3 color = vec3{1});

	static void Line(vec3 beg, vec3 end, vec3 color = vec3{1});
	static void Line(vec3 beg, vec3 end, vec3 bcol, vec3 ecol);

	static void LineUI(vec2 beg, vec2 end, vec3 color = vec3{1});
	static void LineUI(vec2 beg, vec2 end, vec3 bcol, vec3 ecol);
};

#endif