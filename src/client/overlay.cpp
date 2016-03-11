#include "overlay.h"

std::shared_ptr<OverlayManager> OverlayManager::Get() {
	static std::weak_ptr<OverlayManager> wp;
	std::shared_ptr<OverlayManager> p;

	if(!(p = wp.lock()))
		wp = (p = std::make_shared<OverlayManager>());

	return p;
}

void OverlayManager::Add(std::shared_ptr<Overlay> o) {
	overlays.emplace_back(o);
}

void OverlayManager::Update() {
	for(auto& wo: overlays) {
		auto o = wo.lock();
		if(o && o->active) o->Update();
	}

	// Release those that wish death
	auto end = overlays.end();
	overlays.erase(std::remove_if(overlays.begin(), end, 
		[](const std::weak_ptr<Overlay>& wo) { auto o = wo.lock(); return !o || o->wishesDeath; }), end);

	// Higher priority renders last
	std::sort(overlays.begin(), overlays.end(), 
		[](const std::weak_ptr<Overlay>& a, const std::weak_ptr<Overlay>& b) {
			return a.lock()->priority < b.lock()->priority;
	});
}

void OverlayManager::Render() {
	for(auto& wo: overlays) {
		auto o = wo.lock();
		if(o && o->active) o->Render();
	}
}

