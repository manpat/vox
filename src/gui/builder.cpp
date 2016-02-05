#include "gui/builder.h"
#include "gui/element.h"

GUIBuilder::~GUIBuilder() {
	if(vbo) glDeleteBuffers(1, &vbo);
}

void GUIBuilder::Init() {
	vertices.reserve(128);

	glGenBuffers(1, &vbo);
}

void GUIBuilder::Clear() {
	vertices.clear();
}

void GUIBuilder::Upload() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GUIBuilder::Vertex) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GUIBuilder::Add(vec2 p, vec2 uv) {
	vertices.push_back({vec3{p, depth}, uv});
}

void GUIBuilder::AddQuad(CalculatedElementMetrics* m) {
	auto bl = vec2{m->bottomLeft};
	auto tr = vec2{m->topRight};
	auto br = vec2{tr.x, bl.y};
	auto tl = vec2{bl.x, tr.y};

	depth = m->depth;
	Add(bl, vec2{0, 0});
	Add(tl, vec2{0, 1});
	Add(tr, vec2{1, 1});
	Add(br, vec2{1, 0});
}

u32 GUIBuilder::Count() {
	return vertices.size();
}
