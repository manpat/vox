#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

#include "common.h"
#include <vector>

struct Camera;
struct ShaderProgram;
struct OverlayManager;

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
	static u32 vbo;

	static void Init(OverlayManager*);
	static void Render(Camera*);

	static void Point(f32 x, f32 y, f32 z, vec3 color = vec3{1});
	static void Point(vec3 pos, vec3 color = vec3{1});

	static void Line(vec3 beg, vec3 end, vec3 color = vec3{1});
	static void Line(vec3 beg, vec3 end, vec3 bcol, vec3 ecol);
};

#endif