#include "gui.h"
#include "gui/element.h"

#include "camera.h"
#include "quadbuffer.h"
#include "texturehelpers.h"
#include "shaderregistry.h"

static Log logger{"GUI"};

std::shared_ptr<GUI> GUI::Get() {
	static std::weak_ptr<GUI> wp;
	std::shared_ptr<GUI> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<GUI>());

	return p;
}

void GUI::Init() {
	aspect = (f32)screenWidth/screenHeight;

	cellSize = vec2{1.f, 1.f/aspect};
	gridSize = vec2{12.f, 12.f};
	vec2 absSize = cellSize*gridSize;

	auto proj = glm::ortho<f32>(0.f, absSize.x, 0, absSize.y, -10.f, 10.f);
	camera = std::make_shared<Camera>(proj);

	texture = CreateTextureFromFile("textures/ui.png");

	builder.Init();
}

void GUI::Update() {
	auto end = elements.end();
	elements.erase(std::remove_if(elements.begin(), end, 
		[](const auto& p) {return p.expired();}), end);
	
	std::sort(elements.begin(), elements.end(), [](auto& a, auto& b) {
		return a.lock()->depth < b.lock()->depth;
	});

	builder.Clear();

	for(auto& wel: elements) {
		auto el = wel.lock();
		if(el->active) el->ConcreteUpdate();
	}
}

void GUI::Render() {
	// glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	auto sh = ShaderRegistry::GetProgram("ui");
	sh->Use();
	camera->SetUniforms(sh.get());

	if(texture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		if(auto u = sh->GetUniform("tex")) {
			glUniform1i(u, 0);
		}
	}

	builder.Upload();
	glBindBuffer(GL_ARRAY_BUFFER, builder.vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GUIBuilder::Vertex), nullptr); // vertex
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(GUIBuilder::Vertex), (void*)sizeof(GUIBuilder::Vertex::position)); // uv

	QuadElementBuffer::Draw(builder.Count()/4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for(auto& wel: elements) {
		if(auto el = wel.lock())
			if(el->active) el->ConcreteRender();
	}

	glEnable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GUI::AddElement(std::shared_ptr<Element> e) {
	elements.emplace_back(e);
	e->self = e;
}

std::shared_ptr<Element> GUI::GetElementAt(vec2 p) {
	for(auto& we: elements) {
		auto e = we.lock();
		if(!e) continue;

		if(auto res = e->TestPoint(p))
			return res;
	}

	return nullptr;
}

void GUI::InjectMouseMove(vec2 p) {
	auto p4 = vec4{p,0,1};
	p4 = glm::inverse(camera->projectionMatrix) * p4;
	p = p4.xy();

	auto he = hoveredElement.lock();
	auto el = GetElementAt(p);

	if(he != el) {
		if(he) he->OnMouseLeave();
		if(el) el->OnMouseEnter();
	}

	hoveredElement = el;
}

void GUI::InjectMouseButton(bool down) {
	auto se = selectedElement.lock();
	auto he = hoveredElement.lock();

	if(he) {
		if(down) {
			he->OnMouseDown();
		}else{
			he->OnMouseUp();
			if(se == he) he->OnClick();
		}
	}

	selectedElement = he;
}
