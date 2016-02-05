#ifndef OVERLAY_H
#define OVERLAY_H

#include "common.h"

struct Overlay;

struct OverlayManager {
	std::vector<std::shared_ptr<Overlay>> overlays;

	void Add(std::shared_ptr<Overlay>);

	void Update();
	void Render();
};

struct Overlay {
	s32 priority = 0;
	bool wishesDeath = false;
	bool active = true;

	virtual ~Overlay() {}
	virtual void Update() {};
	virtual void Render() {};
};

#endif