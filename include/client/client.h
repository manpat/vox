#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#include <SDL2/SDL.h>

struct OverlayManager;
struct ChunkManager;
struct LocalPlayer;
struct Network;
struct Camera;
struct GUI;

struct Client {
	static constexpr u32 WindowWidth = 800;
	static constexpr u32 WindowHeight = 600;

	SDL_Window* window;
	SDL_GLContext glctx;

	std::shared_ptr<Camera> camera;
	std::shared_ptr<Network> network;
	std::shared_ptr<LocalPlayer> player;
	std::shared_ptr<ChunkManager> chunkManager;
	std::shared_ptr<OverlayManager> overlayManager;
	std::shared_ptr<GUI> gui;

	bool running = true;

	Client();
	~Client();

	void Init();
	void Run();
};

#endif