// Implements Input, a class providing convenient event handling using SDL
 
#include "input.h"
#include <sdl.h>
#include <math.h>

namespace msc {  namespace platform {

// Default constructor
Input::Input() : mSDL(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER),
                 mMouseMovement(Vector2i::Zero()), mMouseWheel(Vector2i::Zero())
{
	SDL_GetMouseState(&mMousePosition.x(), &mMousePosition.y());

	for (uint32_t i = 0; i < SDL_NUM_SCANCODES; i++)
	{
		mKeys[i].PressedState = NotPressed;
	}
	for (uint32_t i = 0; i < NUM_MOUSE_BUTTONS; i++)
	{
		mMouseButtons[i].PressedState = NotPressed;
	}

  UpdateAttachedControllers();
}


//---------------------------------------------------------------------------------------------------------------------

// Return true if the given key has had a key down event since the last call to this function.
// This function will only return true once for each press of a key, in contrast to @ref KeyHeld.
// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the key was pressed
// Parameter: windowID (optional),  only return true if the key was pressed when the given window had focus 
bool Input::KeyHit(SDL_Keycode key, SDL_Keymod* modKeys /*= nullptr*/, uint32_t windowID /*= 0*/)
{
	SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
	if (mKeys[scancode].PressedState != Pressed || (windowID != 0 && mKeys[scancode].WindowID != windowID))  return false;
	if (modKeys)  *modKeys = mKeys[scancode].ModKeys;
	mKeys[scancode].PressedState = Held;
	return true;
}

// Return true if the given key is currently held down. See KeyHit.
// 
// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the key was pressed.
// Parameter: windowID (optional),  only return true if the key was pressed when the given window had focus 
bool Input::KeyHeld(SDL_Keycode key, SDL_Keymod* modKeys /*= nullptr*/, uint32_t windowID /*= 0*/)
{
	SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
	if (mKeys[scancode].PressedState == NotPressed || (windowID != 0 && mKeys[scancode].WindowID != windowID))  return false;
	if (modKeys)  *modKeys = mKeys[scancode].ModKeys;
	mKeys[scancode].PressedState = Held;
	return true;
}


//---------------------------------------------------------------------------------------------------------------------

// Return value > 0 if given button has had a down event in the given window since the last call to this function. The
// value is the initial number of clicks (1=single, 2=double). Will only return a non-zero value once for each click, in
// contrast to MouseHeld.
// 
// Parameter: button,  SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1 or SDL_BUTTON_X2
// Parameter: point (optional),     pointer to variable to recieve the mouse position when the button was clicked
// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the button was pressed
// Parameter: windowID (optional),  only return true if the button was pressed when the given window had focus 
uint32_t Input::MouseClick(uint32_t button, Vector2i* point /*= nullptr*/, SDL_Keymod* modKeys /*= nullptr*/, uint32_t windowID /*= 0*/)
{
	if (mMouseButtons[button].PressedState != Pressed || (windowID != 0 && mMouseButtons[button].WindowID != windowID))  return false;
	if (point)    *point   = mMouseButtons[button].Point;
	if (modKeys)  *modKeys = mMouseButtons[button].ModKeys;
	mMouseButtons[button].PressedState = Held;
	return mMouseButtons[button].Clicks;
}

// Return value > 0 if given button currently held down in the given window. The value is the initial number of clicks
// (1=single, 2=double)
// 
// Parameter: button  SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1 or SDL_BUTTON_X2
// Parameter: point (optional),     pointer to variable to recieve the mouse position when the button was clicked
// Parameter: modKeys (optional),   pointer to an SDL_Keymod variable to get the modifier keys active as the button was pressed
// Parameter: windowID (optional),  only return true if the button was pressed when the given window had focus 
uint32_t Input::MouseHeld(uint32_t button, Vector2i* point /*= nullptr*/, SDL_Keymod* modKeys /*= nullptr*/, uint32_t windowID /*= 0*/)
{
	if (mMouseButtons[button].PressedState == NotPressed || (windowID != 0 && mMouseButtons[button].WindowID != windowID))  return false;
	if (point)    *point   = mMouseButtons[button].Point;
	if (modKeys)  *modKeys = mMouseButtons[button].ModKeys;
	mMouseButtons[button].PressedState = Held;
	return mMouseButtons[button].Clicks;
}


//---------------------------------------------------------------------------------------------------------------------

// Return movement of the mouse since the last call to this function
Vector2i Input::GetMouseMovement()
{
	Vector2i movement = mMouseMovement;
	mMouseMovement = Vector2i::Zero();
	return movement;
}

// Return movement of the mouse wheel since the last call to this function
Vector2i Input::GetMouseWheelMovement()
{
	Vector2i wheel = mMouseWheel;
	mMouseWheel = Vector2i::Zero();
	return wheel;
}


//---------------------------------------------------------------------------------------------------------------------

// Report a new keyboard event
void Input::KeyEvent(const SDL_KeyboardEvent& event)
{
	if (event.type == SDL_KEYDOWN && event.repeat == 0) // Ignore SDL generated keyboard repeat
	{
		mKeys[event.keysym.scancode].PressedState = Pressed;
		mKeys[event.keysym.scancode].WindowID     = event.windowID;
		mKeys[event.keysym.scancode].ModKeys      = SDL_GetModState();
	}
	else if (event.type == SDL_KEYUP)
	{
		mKeys[event.keysym.scancode].PressedState = NotPressed;
	}
}

// Report a new mouse button event
void Input::MouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.which != SDL_TOUCH_MOUSEID) // Ignore touch events
	{
		mMouseButtons[event.button].PressedState = Pressed;
		mMouseButtons[event.button].Point        = Vector2i(event.x, event.y);
		mMouseButtons[event.button].Clicks       = event.clicks;
		mMouseButtons[event.button].WindowID     = event.windowID;
		mMouseButtons[event.button].ModKeys      = SDL_GetModState();
	}
	else if (event.type == SDL_MOUSEBUTTONUP)
	{
		mMouseButtons[event.button].PressedState = NotPressed;
	}
}

// Report a new mouse motion event
void Input::MouseMotionEvent(const SDL_MouseMotionEvent& event)
{
	if (event.type == SDL_MOUSEMOTION  && event.which != SDL_TOUCH_MOUSEID) // Ignore touch events
	{
		mMousePosition = Vector2i(event.x, event.y);
		mMouseMovement += Vector2i(event.xrel, event.yrel);
	}
}

// Report a new mouse wheel event
void Input::MouseWheelEvent(const SDL_MouseWheelEvent& event)
{
	if (event.type == SDL_MOUSEWHEEL && event.which != SDL_TOUCH_MOUSEID) // Ignore touch events
	{
		mMouseWheel += Vector2i(event.x, event.y);
	}
}


//---------------------------------------------------------------------------------------------------------------------

// Update list of attached controllers. Will not invalidate existing controller pointers
void Input::UpdateAttachedControllers()
{
  // Relying on the fact that controller pointers do not change when devices are added/removed or on multiple opens
  // Clear all existing controllers
  for (auto controller : mControllers)  SDL_GameControllerClose(controller);
  mControllers.clear();

  // Fill list again from scratch
  uint32_t numJoysticks = SDL_NumJoysticks();
  for (uint32_t i = 0; i < numJoysticks; i++)
  {
    if (SDL_IsGameController(i))
    {
      SDL_GameController* controller = SDL_GameControllerOpen(i);
      if (controller)  mControllers.push_back(controller);
    }
  }
}

// Convenience function similar to SDL_GameControllerGetAxis that returns the given controller axis
// in floating point range [-1,1], or [0,1] for triggers. Optionally provide a dead zone
float Input::ControllerGetNormalisedAxis(SDL_GameController* controller, SDL_GameControllerAxis axis,
                                         uint32_t deadZone /*= 0*/)
{
  auto value = SDL_GameControllerGetAxis(controller, axis);
  if (static_cast<uint32_t>(abs(value)) < deadZone)  value = 0;
  return value / 32767.0f;
}


} } // namespaces
