// Application entry point

#define SDL_MAIN_HANDLED  /// Prevent SDL from redefining main
#include "engine_test.h"
#include <windows.h>
#include <exception>
#include <iostream>
#include <string>

// Main entry-point for this application.
// Returns: Exit-code for the process - EXIT_SUCCESS or EXIT_FAILURE.
int main()
{
	try
	{
		return msc::EngineTest().Run();
	}
	catch (const std::exception& e)
  { 
    // Output to Visual Studio debug window
    OutputDebugString("\nFatal exception: ");
    OutputDebugString(e.what());
    OutputDebugString("\n\n");
  }
	catch (...)  { OutputDebugString("Unknown fatal exception"); }
	return EXIT_FAILURE;
}
