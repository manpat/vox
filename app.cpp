#include "app.h"
#include "input.h"

static Log logger{"App"};

App::App() {}
App::~App() {}

static void APIENTRY GLDebugCallback(u32, u32, u32, u32, s32, const char*, void*);

void App::Init() {
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
		throw "SDL Init failed";

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
	glDebugMessageCallbackARB(GLDebugCallback, nullptr);
}

void App::Run() {	
	while(running) {
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

		Input::Update();

		if(Input::GetKeyDown(SDLK_ESCAPE))
			running = false;

		glClearColor(0.1,0.1,0.1,0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		SDL_GL_SwapWindow(window);
		SDL_Delay(1);

		Input::EndFrame();
	}
}

void APIENTRY GLDebugCallback(u32 source, u32 type, u32 id, u32 severity, s32 length, const char* msg, void*) {
	if(type != GL_DEBUG_TYPE_ERROR_ARB && type != GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) return;

	static Log gllogger{"GLDebug"};
	gllogger << msg;

	(void) source;
	(void) id;
	(void) severity;
	(void) length;
}