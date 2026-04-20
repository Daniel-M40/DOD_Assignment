// Declares Window, a simple wrapper to encapsulate SDL window creation and deletion
// Contains an implicit cast to SDL_Window* so this object can be used directly in SDL_ window calls

#ifndef MSC_PLATFORM_WINDOW_H_INCLUDED
#define MSC_PLATFORM_WINDOW_H_INCLUDED

#include "sdl_init.h"
#include <sdl_syswm.h>
#include <sdl.h>
#include <string>

namespace msc {  namespace platform {

// Platform independent window class using SDL.
//
// Does not support copy semantics.
class Window
{
	//---------------------------------------------------------------------------------------------------------------------
  // Construction
	//---------------------------------------------------------------------------------------------------------------------
public:
  // Default constructor
  // Parameter: title (optional), window title
  // Parameter: show (optional), true if the window should be shown immediately
  // Parameter: width (optional),  initial window width, set 0 for default width
  // Parameter: height (optional), initial window height, set 0 for default height
  // Throws: std::runtime_error if SDL video subsystem or window cannot be created
	Window(std::string title = "Window", bool show = true, uint32_t width = 0, uint32_t height = 0);

	~Window();

  // Prevent copy/move/assignment (until implemented)
  Window(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(const Window&) = delete;
  Window& operator=(Window&&) = delete;


	//---------------------------------------------------------------------------------------------------------------------
  // Public Interface
	//---------------------------------------------------------------------------------------------------------------------
public:
  // Implicit cast to a SDL_Window pointer so can use this object as a parameter to SDL_ window calls
  operator SDL_Window*() { return mWindow; }

  // Return platform-specific data about the window, primarily window identifiers
  bool PlatformInfo(SDL_SysWMinfo* info)
  {
    SDL_VERSION(&info->version);
    return (SDL_GetWindowWMInfo(mWindow, info) == SDL_TRUE);
  }


	//---------------------------------------------------------------------------------------------------------------------
  // Data
	//---------------------------------------------------------------------------------------------------------------------
private:
	SDL mSDL;

	SDL_Window* mWindow; // SDL cross-platform window
};


} } // namespaces

#endif // Header guard
