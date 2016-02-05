#include "app.h"
#include "gui.h"
#include "block.h"
#include "input.h"
#include "shader.h"
#include "camera.h"
#include "player.h"
#include "physics.h"
#include "overlay.h"
#include "debugdraw.h"
#include "voxelchunk.h"
#include "chunkmanager.h"
#include "bullethelpers.h"
#include "textrendering.h"
#include "shaderregistry.h"

#include "overlays/playerinfo.h"

#include <SDL2/SDL_image.h>
#include <chrono>

static Log logger{"App"};

f32 Time::dt = 0.016f;
f32 Time::time = 0.f;

App::App() {}
App::~App() {}

static void APIENTRY GLDebugCallback(u32, u32, u32, u32, s32, const char*, const void*);

void App::Init() {
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
		throw "SDL Init failed";

	if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		throw "SDL_image init failed";
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	// TODO: Check if debug context available
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	window = SDL_CreateWindow("VoxPhys", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
		WindowWidth, WindowHeight, SDL_WINDOW_OPENGL);
	if(!window) throw "Window create failed";

	glctx = SDL_GL_CreateContext(window);
	if(!glctx) throw "OpenGL context creation failed";

	// I need dat state, yo'
	u32 vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB((GLDEBUGPROCARB) GLDebugCallback, nullptr);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShaderRegistry::CreateProgram("text", "shaders/text.vs", "shaders/text.fs");
	ShaderRegistry::CreateProgram("voxel", "shaders/voxel.vs", "shaders/voxel.fs");
	// ShaderRegistry::CreateProgram("voxel", "shaders/voxel.vs", "shaders/oldvoxel.fs");

	Input::doCapture = true;
	Physics::Init();

	gui = GUI::Get();
	gui->screenWidth = WindowWidth;
	gui->screenHeight = WindowHeight;
	gui->Init();
	
	camera = std::make_shared<Camera>();
	Camera::mainCamera = camera;

	chunkManager = std::make_unique<ChunkManager>();
	overlayManager = std::make_unique<OverlayManager>();

	Debug::Init(overlayManager.get());

	player = std::make_shared<Player>(camera);
}

#include "gui/element.h"

void App::Run() {
	struct TestEl : Element {
		vec3 color;
		TestEl(vec3 c) : color{c} {}

		void Render() override {
			auto m = GetMetrics();

			auto bl = vec3{m->bottomLeft, 0};
			auto tr = vec3{m->topRight, 0};
			auto br = vec3{tr.x, bl.y, 0};
			auto tl = vec3{bl.x, tr.y, 0};

			Debug::Line(bl,tl, color);
			Debug::Line(bl,br, color);
			Debug::Line(tr,tl, color);
			Debug::Line(tr,br, color);
		}
	};

	struct GridEl : Element {
		vec3 gridCol;
		f32 z;
		GridEl(f32 zz = -.1f, vec3 c = vec3{.3}) : gridCol{c}, z{zz} {}
		void Render() override {
			auto m = GetMetrics();
			auto gui = GUI::Get();

			auto bl = vec3{m->bottomLeft, 0};
			auto tr = vec3{m->topRight, 0};
			vec2 cellSize = vec2{tr-bl}/gui->gridSize;

			for(f32 y = bl.y + cellSize.y; y < tr.y; y+=cellSize.y)
				Debug::Line(vec3{bl.x,y,z}, vec3{tr.x,y,z}, gridCol);

			for(f32 x = bl.x + cellSize.x; x < tr.x; x+=cellSize.x)
				Debug::Line(vec3{x,bl.y,z}, vec3{x,tr.y,z}, gridCol);
		}
	};

	auto screenEl = gui->CreateElement<TestEl>(vec3{0,0,1});
	screenEl->position = vec2{0,0};
	screenEl->proportions = vec2{12,12};

	auto gridEl = screenEl->CreateChild<GridEl>(-.2f, vec3{.2f});
	gridEl->position = vec2{0,0};
	gridEl->proportions = vec2{12,12};

	auto rootEl = gui->CreateElement<TestEl>(vec3{1});
	auto childEl = rootEl->CreateChild<TestEl>(vec3{1,1,0});
	auto child2El = rootEl->CreateChild<TestEl>(vec3{0,1,1});
	auto buttonEl = rootEl->CreateChild<TestEl>(vec3{1,0,0});
	auto button2El = rootEl->CreateChild<TestEl>(vec3{0,1,0});

	rootEl->SetOrigin(0, 0);
	rootEl->position = vec2{6,6};
	rootEl->proportions = vec2{10,10};

	childEl->SetOrigin(-1,1);
	childEl->position = vec2{1,11};
	childEl->proportions = vec2{6,6};

	child2El->SetOrigin(1,1);
	child2El->position = vec2{11,11};
	child2El->proportions = vec2{3.5,6};

	buttonEl->SetOrigin(1,-1);
	buttonEl->position = vec2{9, 0.5};
	buttonEl->proportions = vec2{2,1};

	button2El->SetOrigin(1,-1);
	button2El->position = vec2{11.5, 0.5};
	button2El->proportions = vec2{2,1};

	std::vector<std::shared_ptr<TestEl>> longEls {
		childEl->CreateChild<TestEl>(vec3{1,.5,.5}),
		childEl->CreateChild<TestEl>(vec3{1,1,.5}),
		childEl->CreateChild<TestEl>(vec3{.5,1,.5}),
		childEl->CreateChild<TestEl>(vec3{.5,1,1}),
		childEl->CreateChild<TestEl>(vec3{.5,.5,1}),
		childEl->CreateChild<TestEl>(vec3{1,.5,1}),
	};

	for(u32 i = 0; i < longEls.size(); i++) {
		longEls[i]->SetOrigin(-1,1);
		longEls[i]->position = vec2{1, 11 - i*1.5};
		longEls[i]->proportions = vec2{10, 1};
	}

	Font font;
	font.Init("LiberationMono-Regular.ttf");
	Font::defaultFont = &font;

	{	auto chunk = chunkManager->CreateChunk(30,30,10,vec3{0,10,0});

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(x,y,0, 1);
	}

	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;

	auto begin = high_resolution_clock::now();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	overlayManager->Add(std::make_shared<PlayerInfoOverlay>(player));

	while(running) {
		Input::EndFrame();

		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: running = false; break;
				case SDL_KEYDOWN:
				case SDL_KEYUP: 
					if(e.key.repeat == 0) // Ignore repeats
						Input::NotifyKeyStateChange(e.key.keysym.sym, e.key.state == SDL_PRESSED);
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: 
					Input::NotifyButtonStateChange(e.button.button, e.button.state == SDL_PRESSED);
					break;
			}
		}

		Input::Update(window);

		if(Input::GetKeyDown(SDLK_ESCAPE))
			running = false;

		// Toggle snapping mouse to center
		if(Input::GetKeyDown(SDLK_F2))
			Input::doCapture ^= true;

		if(Input::GetKeyDown(SDLK_n)) {
			chunkManager->CreateChunk(11,11,11,camera->position + camera->forward*4.f, true);
		}

		player->Update();
		chunkManager->Update();
		overlayManager->Update();
		gui->Update();

		Physics::world->stepSimulation((btScalar)Time::dt, 10);

		glClearColor(0.1,0.1,0.1,0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		chunkManager->Render(camera.get());
		overlayManager->Render();
		gui->Render();

		SDL_GL_SwapWindow(window);
		SDL_Delay(1);

		auto end = high_resolution_clock::now();
		Time::dt = duration_cast<duration<f32>>(end-begin).count();
		Time::time += Time::dt;
		begin = end;
	}
}

void APIENTRY GLDebugCallback(u32 source, u32 type, u32 id, u32 severity, s32 length, const char* msg, const void*) {
	if(type != GL_DEBUG_TYPE_ERROR_ARB && type != GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) return;

	static Log gllogger{"GLDebug"};
	gllogger << msg;

	(void) source;
	(void) id;
	(void) severity;
	(void) length;
}