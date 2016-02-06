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
	void AddQuad(vec2 bl, vec2 tr, vec2 uv0 = vec2{0.f}, vec2 uv1 = vec2{1.f});
	void Add9Slice(vec2 bl, vec2 tr, vec2 uv, vec2 size, f32 margin = 2.f, f32 frame = 0.1f);
	u32 Count();
};

#endif