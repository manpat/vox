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

#include "gui/panel.h"
#include "gui/label.h"
#include "gui/button.h"

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

	window = SDL_CreateWindow("Vox", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
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

	ShaderRegistry::CreateProgram("ui", "shaders/ui.vs", "shaders/ui.fs");
	ShaderRegistry::CreateProgram("text", "shaders/text.vs", "shaders/text.fs");
	ShaderRegistry::CreateProgram("voxel", "shaders/voxel.vs", "shaders/voxel.fs");

	Input::doCapture = true;
	Physics::Init();

	gui = GUI::Get();
	gui->screenWidth = WindowWidth;
	gui->screenHeight = WindowHeight;
	gui->Init();
	
	camera = std::make_shared<Camera>();
	Camera::mainCamera = camera;

	chunkManager = ChunkManager::Get();
	overlayManager = OverlayManager::Get();

	Debug::Init();

	player = std::make_shared<Player>(camera);
}

void App::Run() {
	Font font;
	font.Init("LiberationMono-Regular.ttf");
	Font::defaultFont = &font;

	auto toolBar = gui->CreateElement<PanelElement>();
	toolBar->SetOrigin(0, -1);
	toolBar->position = vec2{6,0};
	toolBar->proportions = vec2{10,1};

	auto testPanel = gui->CreateElement<PanelElement>();
	testPanel->position = vec2{1,1};
	testPanel->proportions = vec2{10,10};
	testPanel->depth = 1;
	testPanel->active = false;

	auto listPanel = testPanel->CreateChild<PanelElement>();
	listPanel->SetOrigin(1, 1);
	listPanel->position = vec2{11, 11};
	listPanel->proportions = vec2{5,6};
	listPanel->panelSlice = vec2{0, 16};

	std::vector<std::shared_ptr<Element>> childPanels {};
	for(u32 i = 0; i < 4; i++) {
		auto childButton = listPanel->CreateChild<ButtonElement>();
		childButton->SetOrigin(1, 1); // Top right
		childButton->position = vec2{11, 11 - i * 2.5};
		childButton->proportions = vec2{10, 2};

		auto buttonText = childButton->CreateChild<LabelElement>();
		buttonText->textMesh->SetText("Button " + std::to_string(i));
		buttonText->SetOrigin(0,0);
		buttonText->position = vec2{6,6};
		buttonText->proportions = vec2{6,10};

		childPanels.push_back(childButton);
	}

	auto childPanel = testPanel->CreateChild<PanelElement>();
	childPanel->position = vec2{1,1};
	childPanel->proportions = vec2{4,10};
	childPanel->panelSlice = vec2{0, 16};

	auto childPanel2 = childPanel->CreateChild<PanelElement>();
	childPanel2->SetOrigin(0,0);
	childPanel2->position = vec2{6,6};
	childPanel2->proportions = vec2{10,4};

	auto panelText = childPanel2->CreateChild<LabelElement>();
	panelText->SetOrigin(0,0);
	panelText->position = vec2{6,6};
	panelText->proportions = vec2{8,8};

	for(s32 cx = -3; cx <= 3; cx++)
	for(s32 cz = -3; cz <= 3; cz++){
		auto chunk = chunkManager->CreateChunk(30,30,10,vec3{cx*30,10,cz*30});

		for(u32 y = 0; y < chunk->height; y++)
		for(u32 x = 0; x < chunk->width; x++)
			chunk->CreateBlock(ivec3{x,y,0}, 1);
	}

	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;

	auto begin = high_resolution_clock::now();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	auto playerinfo = std::make_shared<PlayerInfoOverlay>(player);
	overlayManager->Add(playerinfo);

	u32 primCount = 0;
	u32 primCountQuery;
	glGenQueries(1, &primCountQuery);

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

		if(Input::GetKeyDown(SDLK_t)){
			testPanel->active ^= true;
			Input::doCapture = !testPanel->active;
		}

		gui->InjectMouseMove(Input::mousePos);
		
		if(Input::GetButtonDown(Input::MouseLeft))
			gui->InjectMouseButton(true);
		else if(Input::GetButtonUp(Input::MouseLeft))
			gui->InjectMouseButton(false);

		player->Update();
		chunkManager->Update();
		overlayManager->Update();
		gui->Update();

		Physics::world->stepSimulation((btScalar)Time::dt, 10);

		glClearColor(0.1,0.1,0.1,0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glBeginQuery(GL_PRIMITIVES_GENERATED, primCountQuery);

		chunkManager->Render(camera.get());
		overlayManager->Render();
		gui->Render();

		Debug::Render();

		glEndQuery(GL_PRIMITIVES_GENERATED);

		SDL_GL_SwapWindow(window);
		SDL_Delay(1);

		glGetQueryObjectuiv(primCountQuery, GL_QUERY_RESULT, &primCount);
		playerinfo->primCount = primCount;

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