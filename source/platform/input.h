// Declares Input, a class providing convenient event handling using SDL

#ifndef MSC_PLATFORM_INPUT_H_INCLUDED
#define MSC_PLATFORM_INPUT_H_INCLUDED

#include "sdl_init.h"
#include <sdl.h>
#include <Eigen/Core>
#include <vector>
#include <stdint.h>
using Eigen::Vector2i;

namespace msc {  namespace platform {

// Platform independent keyboard & mouse event handling using SDL.
//
// Collects SDL keyboard and mouse events and provides convenient functions for polling keyboard & mouse state
// Does not support copy semantics.
class Input
{
	//---------------------------------------------------------------------------------------------------------------------
  // Construction
	//---------------------------------------------------------------------------------------------------------------------
public:
	// Default constructor
	Input();

  // Prevent copy/move/assignment
  Input(const Input&) = delete;
  Input(Input&&) = delete;
  Input& operator=(const Input&) = delete;
  Input& operator=(Input&&) = delete;


	//---------------------------------------------------------------------------------------------------------------------
  // Public Interface
	//---------------------------------------------------------------------------------------------------------------------
public:
	// Return true if the given key has had a key down event since the last call to this function.
	// This function will only return true once for each press of a key, in contrast to @ref KeyHeld.
	// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the key was pressed
	// Parameter: windowID (optional),  only return true if the key was pressed when the given window had focus 
	bool KeyHit(SDL_Keycode key, SDL_Keymod* modKeys = nullptr, uint32_t windowID = 0);

	// Return true if the given key is currently held down. See KeyHit.
	// 
	// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the key was pressed.
	// Parameter: windowID (optional),  only return true if the key was pressed when the given window had focus 
	bool KeyHeld(SDL_Keycode key, SDL_Keymod* modKeys = nullptr, uint32_t windowID = 0);


	//---------------------------------------------------------------------------------------------------------------------

	// Return value > 0 if given button has had a down event in the given window since the last call to this function. The
	// value is the initial number of clicks (1=single, 2=double). Will only return a non-zero value once for each click, in
	// contrast to MouseHeld.
	// 
	// Parameter: button,  SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1 or SDL_BUTTON_X2
	// Parameter: point (optional),     pointer to variable to recieve the mouse position when the button was clicked
	// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the button was pressed
	// Parameter: windowID (optional),  only return true if the button was pressed when the given window had focus 
	uint32_t MouseClick(uint32_t button, Vector2i* point = nullptr, SDL_Keymod* modKeys = nullptr, uint32_t windowID = 0);
	
	// Return value > 0 if given button currently held down in the given window. The value is the initial number of clicks
	// (1=single, 2=double)
	// 
	// Parameter: button  SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1 or SDL_BUTTON_X2
	// Parameter: point (optional),     pointer to variable to recieve the mouse position when the button was clicked
	// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the button was pressed
	// Parameter: windowID (optional),  only return true if the button was pressed when the given window had focus 
	uint32_t MouseHeld(uint32_t button, Vector2i* point = nullptr, SDL_Keymod* modKeys = nullptr, uint32_t windowID = 0);


	//---------------------------------------------------------------------------------------------------------------------

	// Return current position of the mouse within the window
	Vector2i GetMousePosition()  { return mMousePosition; } 

	// Return movement of the mouse since the last call to this function
	Vector2i GetMouseMovement();

	// Return movement of the mouse wheel since the last call to this function
  // Note that some wheels can move left and right, hence the Vector2i return value
	Vector2i GetMouseWheelMovement();


	//---------------------------------------------------------------------------------------------------------------------

	// Report a new keyboard event
	void KeyEvent(const SDL_KeyboardEvent& event);

	// Report a new mouse button event
	void MouseButtonEvent(const SDL_MouseButtonEvent& event);

	// Report a new mouse motion event
	void MouseMotionEvent(const SDL_MouseMotionEvent& event);
	
	// Report a new mouse wheel event
	void MouseWheelEvent(const SDL_MouseWheelEvent& event);


	//---------------------------------------------------------------------------------------------------------------------

  // Update list of attached controllers. Will not invalidate existing controller pointers
  void UpdateAttachedControllers();

  // Retrieve the current list of attached controllers. Use SDL functions directly to use.
  // List will change as controllers are added or removed. Check controllers are attached
  // with SDL_GameControllerGetAttached before use.
  const std::vector<SDL_GameController*>& AttachedControllers()  { return mControllers; }


  // Convenience function similar to SDL_GameControllerGetAxis that returns the given controller axis
  // in floating point range [-1,1], or [0,1] for triggers. Optionally provide a dead zone
  float ControllerGetNormalisedAxis(SDL_GameController* controller, SDL_GameControllerAxis axis,
                                    uint32_t deadZone = 0);


	//---------------------------------------------------------------------------------------------------------------------
  // Types
	//---------------------------------------------------------------------------------------------------------------------
private:
	enum EKeyPressedState 
	{
		NotPressed,
		Pressed,
		Held,
	};

	struct SKeyState
	{
		EKeyPressedState PressedState;
		uint32_t         WindowID; // Window that was active when the key was pressed (no meaning if key is not pressed)
		SDL_Keymod       ModKeys;  // Modifier keys active when the key was pressed (no meaning if key is not pressed)
	};

	struct SMouseButtonState
	{
		EKeyPressedState PressedState;
		Vector2i         Point;    // Cursor position when the button was pressed
		uint32_t         Clicks;   // Number of clicks when the button was pressed (1=single, 2=double)
		uint32_t         WindowID; // Window that was active when the button was pressed (no meaning if key is not pressed)
		SDL_Keymod       ModKeys;  // Modifier keys active when the button was pressed (no meaning if key is not pressed)
	};


	//---------------------------------------------------------------------------------------------------------------------
  // Data
	//---------------------------------------------------------------------------------------------------------------------
private:
  SDL mSDL;

	// Key state data for every scancode. API above uses keycodes rather than scancodes
	SKeyState mKeys[SDL_NUM_SCANCODES];

	// Mouse button state data for SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1 and SDL_BUTTON_X2
	static const uint32_t NUM_MOUSE_BUTTONS = 6; // @todo Assuming SDL values are 1-5 (with 0 unused). Highly unlikely to change
	SMouseButtonState mMouseButtons[NUM_MOUSE_BUTTONS]; 

	// Mouse cursor position and (relative) movement
	Vector2i mMousePosition;
	Vector2i mMouseMovement;

	// Mouse wheel (relative) movement
	Vector2i mMouseWheel;

  std::vector<SDL_GameController*> mControllers;
};

} } // namespaces

#endif // Header guard
