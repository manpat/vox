#ifndef TEXTRENDERING_H
#define TEXTRENDERING_H

#include "common.h"
#include "stb_truetype.h"

struct Font {
	static Font* defaultFont;

	stbtt_packedchar* charData;
	u32 tex;
	f32 ascent;
	f32 descent;

	void Init(const std::string&);
	~Font();
};

struct TextMesh {
	Font* font;
	
	mat4 modelMatrix{1.f};
	vec2 size{0.f};
	f32 lineHeightMult = 1.f;
	
	u32 vbo = 0, tbo = 0;
	u32 quads = 0;

	TextMesh(Font* f = nullptr) : font{f}, lineHeightMult{1.f} {}

	void SetFont(Font*);
	void SetText(const std::string&);
	void Render();
};

#endif