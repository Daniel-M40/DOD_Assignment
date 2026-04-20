// Implements Window, a simple wrapper to encapsulate SDL window creation and deletion
 
#include "window.h"
#include <sdl.h>
#include <stdexcept>

namespace msc {  namespace platform {

// Default constructor
// Parameter: title (optional),  window title
// Parameter: show (optional),   true if the window should be shown immediately
// Parameter: width (optional),  initial window width, set 0 for default width
// Parameter: height (optional), initial window height, set 0 for default height
// Throws: std::runtime_error if SDL video subsystem or window cannot be created
Window::Window(std::string title /*= "Window"*/, bool show /*= true*/, 
               uint32_t width /*= 0*/, uint32_t height /*= 0*/) : mSDL(SDL_INIT_VIDEO)
{
	// Make SDL window, hidden at first since size and content will be updatead
	mWindow = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
	if (mWindow == nullptr)
	{
		throw std::runtime_error(title + " window creation failure");
	}

	// Without a given size make window 3/4 of the desktop size. No window decoration size API in SDL so this is an estimate
	int maxX, maxY;
	SDL_MaximizeWindow(mWindow);
	SDL_GetWindowSize(mWindow, &maxX, &maxY);
	SDL_RestoreWindow(mWindow);
	if (width  == 0)  width =  ((maxX * 3) / 4 + 15) & ~15; // Make client area multiple of 16
	if (height == 0)  height = ((maxY * 3) / 4 + 15) & ~15; // --"--
	SDL_SetWindowSize(mWindow, width, height);
	SDL_SetWindowPosition(mWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// Clear window to black because it starts (partially) white in SDL
	SDL_Surface* surface = SDL_GetWindowSurface(mWindow);
	if (surface != nullptr) // Don't worry about problems with this small nicety
	{
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
		SDL_UpdateWindowSurface(mWindow);
		SDL_FreeSurface(surface);
	}
	if (show)  SDL_ShowWindow(mWindow);
}


Window::~Window()
{
	SDL_DestroyWindow(mWindow);
}


}  } // namespaces
