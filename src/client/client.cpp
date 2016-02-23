#include "gui.h"
#include "block.h"
#include "input.h"
#include "client.h"
#include "shader.h"
#include "camera.h"
#include "network.h"
#include "physics.h"
#include "overlay.h"
#include "debugdraw.h"
#include "voxelchunk.h"
#include "localplayer.h"
#include "chunkmanager.h"
#include "playermanager.h"
#include "chunkrenderer.h"
#include "bullethelpers.h"
#include "textrendering.h"
#include "shaderregistry.h"
#include "clientnetinterface.h"

#include "overlays/playerinfo.h"

#include "gui/panel.h"
#include "gui/label.h"
#include "gui/button.h"

#include <SDL2/SDL_image.h>
#include <chrono>

static Log logger{"Client"};

f32 Time::dt = 0.016f;
f32 Time::time = 0.f;

Client::Client() {}
Client::~Client() {}

static void APIENTRY GLDebugCallback(u32, u32, u32, u32, s32, const char*, const void*);

void Client::Init() {
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

	network = Network::Get();
	network->Init();
	network->Connect("localhost", 16660);

	gui = GUI::Get();
	gui->screenWidth = WindowWidth;
	gui->screenHeight = WindowHeight;
	gui->Init();
	
	camera = std::make_shared<Camera>(gui->aspect);
	Camera::mainCamera = camera;

	chunkManager = ChunkManager::Get();
	overlayManager = OverlayManager::Get();

	Debug::Init();

	for(u32 i = 0; i < 100 && !network->isConnected; i++) {
		network->Update();
		SDL_Delay(10);
	}

	if(!network->isConnected) throw "Connection failed";

	camera->position = vec3{2.5, 10, 2.5};
	player = std::make_shared<LocalPlayer>(camera);
	playerManager = PlayerManager::Get();
	playerManager->AddPlayer(player, 0);
}

void Client::Run() {
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

	auto chunkRenderer = std::make_shared<ChunkRenderer>();

	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;

	auto begin = high_resolution_clock::now();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	auto playerinfo = std::make_shared<PlayerInfoOverlay>(player);
	overlayManager->Add(playerinfo);

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
					Input::hasFocus = true;
					Input::NotifyButtonStateChange(e.button.button, e.button.state == SDL_PRESSED);
					break;

				case SDL_WINDOWEVENT:
					if(e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
						Input::hasFocus = false;
					break;
			}
		}

		Input::Update(window);

		if(Input::GetKeyDown(SDLK_ESCAPE))
			running = false;

		// Relinquish focus
		if(Input::GetKeyDown(SDLK_F2))
			Input::hasFocus = false;

		if(Input::GetKeyDown(SDLK_n)) {
			auto ch = chunkManager->CreateChunk(11,11,11);

			auto chpos = camera->position + camera->forward*4.f;
			chpos.x += 11.f/-2.f -1;
			chpos.y += 11.f/-2.f -1;
			chpos.z += 11.f/ 2.f +1;
			// ch->modelMatrix = glm::translate(chpos);
			ch->position = chpos;

			auto vx = ch->WorldToVoxelSpace(camera->position + camera->forward*4.f);
			ch->CreateBlock(vx, 1);
		}

		if(Input::GetKeyDown(SDLK_r))
			ClientNetInterface::RequestRefreshChunks();

		if(Input::GetKeyDown(SDLK_t)){
			testPanel->active ^= true;
			Input::doCapture = !testPanel->active;
		}

		if(Input::GetKeyDown(SDLK_DELETE)) {
			chunkManager->chunks.clear();
		}

		ClientNetInterface::Update(network);

		// TODO: Move into a player state
		gui->InjectMouseMove(Input::mousePos);
		if(Input::GetButtonDown(Input::MouseLeft))
			gui->InjectMouseButton(true);
		else if(Input::GetButtonUp(Input::MouseLeft))
			gui->InjectMouseButton(false);

		playerManager->Update();
		chunkManager->Update();
		overlayManager->Update();
		gui->Update();

		Physics::world->stepSimulation((btScalar)Time::dt, 10);

		glClearColor(0.1,0.1,0.1,0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glBeginQuery(GL_PRIMITIVES_GENERATED, primCountQuery);

		playerManager->Render();
		chunkRenderer->Render(); 
		overlayManager->Render();
		gui->Render();

		Debug::Render();

		glEndQuery(GL_PRIMITIVES_GENERATED);

		SDL_GL_SwapWindow(window);
		SDL_Delay(1);

		glGetQueryObjectuiv(primCountQuery, GL_QUERY_RESULT, &playerinfo->primCount);

		auto end = high_resolution_clock::now();
		Time::dt = duration_cast<duration<f32>>(end-begin).count();
		Time::time += Time::dt;
		begin = end;
	}

	network->Shutdown();
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