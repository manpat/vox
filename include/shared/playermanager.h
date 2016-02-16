#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include "common.h"

struct PlayerBase;

struct PlayerManager {
	std::vector<std::shared_ptr<PlayerBase>> players;

	static std::shared_ptr<PlayerManager> Get();

	void AddPlayer(std::shared_ptr<PlayerBase>, u16 id);
	void RemovePlayer(u16 id);
	std::shared_ptr<PlayerBase> GetPlayer(u16 id);

	void Update();
	void Render();
};

#endif