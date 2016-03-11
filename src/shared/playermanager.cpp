#include "playermanager.h"
#include "playerbase.h"

std::shared_ptr<PlayerManager> PlayerManager::Get() {
	static std::weak_ptr<PlayerManager> wp;
	std::shared_ptr<PlayerManager> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<PlayerManager>());

	return p;
}


void PlayerManager::AddPlayer(std::shared_ptr<PlayerBase> p, u16 id) {
	p->playerID = id;
	players.push_back(p);
}

void PlayerManager::RemovePlayer(u16 id) {
	auto p = std::find_if(players.begin(), players.end(), 
		[id](const std::shared_ptr<PlayerBase>& p) {
			return id == p->playerID;
	});

	players.erase(p);
}

std::shared_ptr<PlayerBase> PlayerManager::GetPlayer(u16 id) {
	auto p = std::find_if(players.begin(), players.end(), 
		[id](const std::shared_ptr<PlayerBase>& p) {
			return id == p->playerID;
	});

	if(p == players.end()) return nullptr;
	return *p;
}

void PlayerManager::Update() {
	for(auto& p : players) {
		p->Update();
	}	
}

void PlayerManager::Render() {
	for(auto& p : players) {
		p->Render();
	}
}
