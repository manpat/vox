#include "overlay.h"

void OverlayManager::Add(std::shared_ptr<Overlay> o) {
	overlays.push_back(o);
}

void OverlayManager::Update() {
	for(auto& o: overlays) {
		if(o->active) o->Update();
	}

	// Release those that wish death
	auto end = overlays.end();
	overlays.erase(std::remove_if(overlays.begin(), end, 
		[](const auto& o) { return !o || o->wishesDeath; }), end);

	// Higher priority renders last
	std::sort(overlays.begin(), overlays.end(), [](const auto& a, const auto& b) {
		return a->priority < b->priority;
	});
}

void OverlayManager::Render() {
	for(auto& o: overlays) {
		if(o->active) o->Render();
	}
}

