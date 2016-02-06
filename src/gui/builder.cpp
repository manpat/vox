#include "gui/builder.h"
#include "gui/element.h"
#include "gui.h"

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

void GUIBuilder::AddQuad(vec2 bl, vec2 tr, vec2 uv00, vec2 uv11) {
	auto br = vec2{tr.x, bl.y};
	auto tl = vec2{bl.x, tr.y};

	auto uv01 = vec2{uv00.x, uv11.y};
	auto uv10 = vec2{uv11.x, uv00.y};

	Add(bl, uv00);
	Add(tl, uv01);
	Add(tr, uv11);
	Add(br, uv10);
}

void GUIBuilder::Add9Slice(vec2 bl, vec2 tr, vec2 uv, vec2 size, f32 margin, f32 frame) {
	vec2 frameSize = vec2{1, GUI::Get()->aspect} * frame;

	f32 tx[4] = {uv.x, uv.x+margin, uv.x+size.x-margin, uv.x+size.x};
	f32 ty[4] = {uv.y+size.y, uv.y+size.y-margin, uv.y+margin, uv.y};

	f32 px[4] = {bl.x, bl.x+frameSize.x, tr.x-frameSize.x, tr.x};
	f32 py[4] = {bl.y, bl.y+frameSize.y, tr.y-frameSize.y, tr.y};

	for(u8 y = 0; y < 3; y++)
	for(u8 x = 0; x < 3; x++) {
		auto p0 = vec2{px[x], py[y]};
		auto t0 = vec2{tx[x], ty[y]};

		auto p1 = vec2{px[x+1], py[y+1]};
		auto t1 = vec2{tx[x+1], ty[y+1]};

		AddQuad(p0, p1, t0, t1);
	}
}

u32 GUIBuilder::Count() {
	return vertices.size();
}
