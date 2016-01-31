#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "common.h"

// TODO: Text input
// TODO: Scroll wheel input

// Required for mouse warping
struct SDL_Window;

class Input {
public:
	enum MappingName {
		Select,
		Cancel,
		Forward,
		Backward,
		Left,
		Right,
		Boost,
		Jump,
		Interact,
		Count
	};

	struct MappedCode {
		s32 keyCode = -1;
		s32 mouseCode = -1;

		MappedCode(s32 k, s32 m) : keyCode{k}, mouseCode{m} {}
	};

	enum {
		Up = 0,
		Down = 1,

		// This flag is for indicating that a key changed during a frame
		//	Can be used for triggering things that should only happen once per
		//	key press.
		ChangedThisFrameFlag = 1<<2
	};

	enum {
		MouseLeft = SDL_BUTTON_LEFT,
		MouseMiddle = SDL_BUTTON_MIDDLE,
		MouseRight = SDL_BUTTON_RIGHT,
	};

	static std::map<s32, u8> keyStates;
	static std::map<s32, u8> mouseStates;
	static vec2 mousePos;
	static vec2 mouseDelta;
	static MappedCode mappings[MappingName::Count];

	static bool doCapture;

	// Returns mouse delta since last frame
	static vec2 GetMouseDelta();

	// Returns if a mouse button is pressed
	static bool GetButton(s32 k);

	// Returns if a mouse button was pressed this frame
	static bool GetButtonDown(s32 k);

	// Returns if a mouse button was released this frame
	static bool GetButtonUp(s32 k);


	// Returns if a key is pressed
	static bool GetKey(s32 k);

	// Returns if a key was pressed this frame
	static bool GetKeyDown(s32 k);

	// Returns if a key was released this frame
	static bool GetKeyUp(s32 k);

	static bool GetMapped(s32 k);
	static bool GetMappedDown(s32 k);
	static bool GetMappedUp(s32 k);

	// Update methods
	static void Update(SDL_Window*);
	static void EndFrame();

	static void NotifyKeyStateChange(s32 key, bool state);
	static void NotifyButtonStateChange(s32 key, bool state);
};

#endif