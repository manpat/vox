#ifndef GUI_BUILDER_H
#define GUI_BUILDER_H

#include "common.h"

struct CalculatedElementMetrics;

// TODO: How do deal with depth?
struct GUIBuilder {
	struct Vertex {
		vec3 position;
		vec2 uv; // UVW for texture array?
	};

	std::vector<Vertex> vertices;
	u32 vbo;
	f32 depth;

	~GUIBuilder();

	void Init();
	void Clear();
	void Upload();

	void Add(vec2 p, vec2 uv = vec2{0.f});
	void AddQuad(CalculatedElementMetrics* m);
	u32 Count();
};

#endif