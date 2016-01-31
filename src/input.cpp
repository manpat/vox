#include "common.h"
#include "input.h"
#include "app.h"

std::map<s32,u8> Input::keyStates;
std::map<s32,u8> Input::mouseStates;
vec2 Input::mouseDelta = vec2{0.f};
vec2 Input::mousePos = vec2{0.f};
bool Input::doCapture = true;

Input::MappedCode Input::mappings[MappingName::Count] = {
	// Keyboard, Mouse
	{SDLK_RETURN, -1},			// Select
	{SDLK_ESCAPE, -1},			// Cancel
	{SDLK_w, -1},				// Forward
	{SDLK_s, -1},				// Backward
	{SDLK_a, -1},				// Left
	{SDLK_d, -1},				// Right
	{SDLK_LSHIFT, -1},			// Boost
	{SDLK_SPACE, -1},			// Jump
	{SDLK_e, SDL_BUTTON_LEFT}	// Interact
};

/*

	88        88                         88
	88        88                         88
	88        88                         88
	88aaaaaaaa88  ,adPPYba,   ,adPPYba,  88   ,d8  ,adPPYba,
	88""""""""88 a8"     "8a a8"     "8a 88 ,a8"   I8[    ""
	88        88 8b       d8 8b       d8 8888[      `"Y8ba,
	88        88 "8a,   ,a8" "8a,   ,a8" 88`"Yba,  aa    ]8I
	88        88  `"YbbdP"'   `"YbbdP"'  88   `Y8a `"YbbdP"'


*/

void Input::Update(SDL_Window* window){
	// Get mouse delta from center, convert to range (-1, 1),
	//	move mouse back to center

	// The reason that this isn't being handled with SDLs event queue
	//	is the mouse warping

	s32 mx, my;
	SDL_GetMouseState(&mx, &my);

	u32 ww = App::WindowWidth;
	u32 wh = App::WindowHeight;

	ww &= ~1;
	wh &= ~1;

	if(doCapture){	
		mouseDelta.x = mx / static_cast<f32>(ww) * 2.f - 1.f;
		mouseDelta.y =-my / static_cast<f32>(wh) * 2.f + 1.f;

		mousePos = vec2{0.f};
		
		SDL_WarpMouseInWindow(window, ww/2, wh/2);
	}else{
		mousePos.x = mx / static_cast<f32>(ww) * 2.f - 1.f;
		mousePos.y =-my / static_cast<f32>(wh) * 2.f + 1.f;

		// Not sure if mouseDelta is useful when not capturing mouse
		mouseDelta = vec2{0.f};
	}
}

void Input::EndFrame(){
	// Clear all ChangedThisFrameFlag's from keyStates
	for(auto& kv: keyStates){
		kv.second &= ~Input::ChangedThisFrameFlag;
	}
	for(auto& kv: mouseStates){
		kv.second &= ~Input::ChangedThisFrameFlag;
	}
}

void Input::NotifyKeyStateChange(s32 key, bool state) {
	keyStates[key] = (state? Input::Down:Input::Up) | Input::ChangedThisFrameFlag;
}

void Input::NotifyButtonStateChange(s32 button, bool state) {
	mouseStates[button] = (state? Input::Down:Input::Up) | Input::ChangedThisFrameFlag;
}


/*

	  ,ad8888ba,
	 d8"'    `"8b              ,d      ,d
	d8'                        88      88
	88             ,adPPYba, MM88MMM MM88MMM ,adPPYba, 8b,dPPYba, ,adPPYba,
	88      88888 a8P_____88   88      88   a8P_____88 88P'   "Y8 I8[    ""
	Y8,        88 8PP"""""""   88      88   8PP""""""" 88          `"Y8ba,
	 Y8a.    .a88 "8b,   ,aa   88,     88,  "8b,   ,aa 88         aa    ]8I
	  `"Y88888P"   `"Ybbd8"'   "Y888   "Y888 `"Ybbd8"' 88         `"YbbdP"'


*/

// Helper
template<typename K, typename V>
V findin(const std::map<K,V>& m, K k, V dv = V()){
	auto it = m.find(k);
	if(it == m.end()) return dv;

	return it->second;
}

vec2 Input::GetMouseDelta(){
	return mouseDelta;
}

bool Input::GetButton(s32 k) {
	// Get only the raw "up or down" state
	return findin(mouseStates, k) & 1;
}
bool Input::GetButtonDown(s32 k) {
	// Get state and return if it is down and has changed this frame
	return findin(mouseStates, k) == (Input::ChangedThisFrameFlag|Input::Down);
}
bool Input::GetButtonUp(s32 k) {
	// Get state and return if it is up and has changed this frame
	return findin(mouseStates, k) == (Input::ChangedThisFrameFlag|Input::Up);
}


bool Input::GetKey(s32 k){
	// Get only the raw "up or down" state
	return findin(keyStates, k) & 1;
}

bool Input::GetKeyDown(s32 k){
	// Get state and return if it is down and has changed this frame
	return findin(keyStates, k) == (Input::ChangedThisFrameFlag|Input::Down);
}

bool Input::GetKeyUp(s32 k){
	// Get state and return if it is up and has changed this frame
	return findin(keyStates, k) == (Input::ChangedThisFrameFlag|Input::Up);
}


bool Input::GetMapped(s32 k) {
	assert(k < Input::Count && k >= 0);

	auto& map = mappings[k];

	if (map.keyCode > 0 && GetKey(map.keyCode)) {
		return true;
	}
	if (map.mouseCode > 0 && GetButton(map.mouseCode)) {
		return true;
	}
	return false;
}

bool Input::GetMappedDown(s32 k) {
	assert(k < Input::Count && k >= 0);

	auto& map = mappings[k];

	if (map.keyCode > 0 && GetKeyDown(map.keyCode)) {
		return true;
	}
	if (map.mouseCode > 0 && GetButtonDown(map.mouseCode)) {
		return true;
	}
	return false;
}

bool Input::GetMappedUp(s32 k) {
	assert(k < Input::Count && k >= 0);

	auto& map = mappings[k];

	if (map.keyCode > 0 && GetKeyUp(map.keyCode)) {
		return true;
	}
	if (map.mouseCode > 0 && GetButtonUp(map.mouseCode)) {
		return true;
	}
	return false;
}