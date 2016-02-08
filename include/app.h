#ifndef APP_H
#define APP_H

#include "common.h"

#include <SDL2/SDL.h>

struct OverlayManager;
struct ChunkManager;
struct Camera;
struct Player;
struct GUI;

struct App {
	static constexpr u32 WindowWidth = 800;
	static constexpr u32 WindowHeight = 600;

	SDL_Window* window;
	SDL_GLContext glctx;

	std::shared_ptr<Player> player;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<ChunkManager> chunkManager;
	std::shared_ptr<OverlayManager> overlayManager;
	std::shared_ptr<GUI> gui;

	bool running = true;

	App();
	~App();

	void Init();
	void Run();
};

#endif