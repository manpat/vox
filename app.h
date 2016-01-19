#ifndef APP_H
#define APP_H

#include "common.h"

#include <SDL2/SDL.h>

struct App {
	static constexpr u32 WindowWidth = 800;
	static constexpr u32 WindowHeight = 600;

	SDL_Window* window;
	SDL_GLContext glctx;

	bool running = true;

	App();
	~App();

	void Init();
	void Run();
};

#endif