#include "quadbuffer.h"
#include "textrendering.h"
#include "shaderregistry.h"
#include <limits>
#include <vector>
#include <fstream>

#include <locale>
#include <codecvt>

constexpr u64 TexWidth = 1024;
constexpr u64 TexHeight = 1024;
constexpr f32 FontSize = 60.f;
constexpr u64 NumChars = 0x7f - 0x20;
constexpr u64 NumCharsSupl = 0xff - 0xa0; // Latin-1 supplement

Font* Font::defaultFont = nullptr;

void Font::Init(const std::string& filename) {
	Log("Font") << "Initialising " << filename;

	std::ifstream file{filename, file.binary};
	if(!file) {
		Log("Font") << "Font file open failed: " + filename;
		return;
	}

	file.seekg(0, file.end);
	u64 size = file.tellg();
	u8* fontData = new u8[size];
	file.seekg(0);
	file.read(reinterpret_cast<char*>(fontData), size);
	file.close();

	static u8 tempBitmap[TexWidth*TexHeight];

	Log("Font") << NumChars;
	charData = new stbtt_packedchar[NumChars+NumCharsSupl];

	stbtt_pack_context pc;
	stbtt_PackBegin(&pc, tempBitmap, TexWidth, TexHeight, 0, 1, nullptr);
	stbtt_PackSetOversampling(&pc, 2, 2);
	stbtt_PackFontRange(&pc, fontData, 0, FontSize, 0x20, NumChars, charData);
	stbtt_PackFontRange(&pc, fontData, 0, FontSize, 0xa1, NumCharsSupl, charData+NumChars);
	stbtt_PackEnd(&pc);

	stbtt_fontinfo fontInfo;
	stbtt_InitFont(&fontInfo, fontData, 0);

	s32 asc, desc;
	stbtt_GetFontVMetrics(&fontInfo, &asc, &desc, nullptr);

	f32 height = asc-desc;
	ascent = asc/height;
	descent = desc/height;

	delete[] fontData;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TexWidth, TexHeight, 0, GL_RED, GL_UNSIGNED_BYTE, tempBitmap);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Font::~Font() {
	delete[] charData;
	charData = nullptr;
}

static Log tmLogger{"TextMesh"};

void TextMesh::SetFont(Font* f) {
	font = f;
}

void TextMesh::SetText(const std::string& text) {
	if(!font) {
		tmLogger << "Tried to set text on TextMesh with no font";
		return;
	}

	if(!vbo) glGenBuffers(1, &vbo);
	if(!tbo) glGenBuffers(1, &tbo);

	if(text.size() > 0) {
		std::vector<vec2> verts;
		std::vector<vec2> uv;

		f32 x = 0.f, y = 0.f;
		size.x = 0.f;

		auto& space = font->charData[0];

		std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> cv;
		
		for(auto c: cv.from_bytes(text)) {
			switch(c) {
			case '\n':
				y += lineHeightMult;
				x = 0.f;
				break;

			case '\t':
				x += space.xadvance/FontSize*4;
				size.x = std::max(x, size.x);
				break;
			}

			if(c < 0x20) continue;

			u32 dataIdx = c-0x20;
			if(c > 0xa0) dataIdx = c-0xa1+NumChars;

			auto& cd = font->charData[dataIdx];
			float u0 = (cd.x0+.5f)/(f32)TexWidth, u1 = (cd.x1+.5f)/(f32)TexWidth;
			float v0 = (cd.y0+.5f)/(f32)TexHeight, v1 = (cd.y1+.5f)/(f32)TexHeight;

			float x0 = cd.xoff/FontSize, x1 = cd.xoff2/FontSize;
			float y0 = cd.yoff/FontSize, y1 = cd.yoff2/FontSize;

			verts.emplace_back(x0+x, -y1-y-font->ascent);
			verts.emplace_back(x0+x, -y0-y-font->ascent);
			verts.emplace_back(x1+x, -y0-y-font->ascent);
			verts.emplace_back(x1+x, -y1-y-font->ascent);

			uv.emplace_back(u0, v1);
			uv.emplace_back(u0, v0);
			uv.emplace_back(u1, v0);
			uv.emplace_back(u1, v1);

			x += cd.xadvance/FontSize;
			size.x = std::max(x, size.x);
		}

		size.y = y+1.f;
		quads = verts.size()/4;

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec2), &verts[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, uv.size()*sizeof(vec2), &uv[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}else{
		quads = 0;
		size = vec2{0.f};
	}

	modelMatrix = glm::scale<f32>(.5,.5,.5) * glm::translate<f32>(-size.x/2.f, size.y/2.f, 0);
}

void TextMesh::Render() {
	if(!quads) return;

	auto program = ShaderRegistry::GetProgram("text");

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, nullptr);

	glBindTexture(GL_TEXTURE_2D, font->tex);
	glUniform1i(program->GetUniform("tex"), 0);
	glUniformMatrix4fv(program->GetUniform("model"), 1, false, glm::value_ptr(modelMatrix));

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Text uses premultiplied alpha
	QuadElementBuffer::Draw(quads);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

}
