// Implements EngineTest, a class to test the sprite engine API

#include "engine_test.h"
#include <condition_variable>
#include <math.h>
#include <sdl.h>
#include <thread>

#include "circle.h"
#include "sim_config.h"
#include "utility.h"
#include "render/engine_dx.h"

namespace msc
{
	std::vector<Circle> circles;
	SimConfig config = {};

	// Constructor initialises SDL to receive input/window events and also creates a (hidden) window
	EngineTest::EngineTest() : mSDL(SDL_INIT_EVENTS), mWindow("Sprite Engine Test", false)
	{
		SDL_SetEventFilter(EventFilter, this);
	}

	EngineTest::~EngineTest()
	{
	}


	//---------------------------------------------------------------------------------------------------------------------

	// Load and pre-process game resources. Returns true on success
	bool EngineTest::SceneSetup()
	{
		circles = std::vector<Circle>(config.numCircles);

		for (int i = 0; i < config.numCircles; i++)
		{
			float worldX = config.worldSize.x();
			float worldY = config.worldSize.y();

			float minVel = config.maxVelocity.x();
			float maxVel = config.maxVelocity.y();

			float x = msc::RandomInRange(-worldX, worldX);
			float y = msc::RandomInRange(-worldY, worldY);

			float velX = msc::RandomInRange(minVel, maxVel);
			float velY = msc::RandomInRange(minVel, maxVel);

			circles[i].position = Vector2f(x, y);
			circles[i].velocity= Vector2f(velX, velY);
			circles[i].colour = Vector3f(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
		}


		return true;
	}


	//---------------------------------------------------------------------------------------------------------------------

	// Main scene update code. Called every frame, uses mFrameTime member for timing
	void EngineTest::SceneUpdate()
	{

		constexpr int MAX_THREADS = 10;
		constexpr int DEFAULT_THREADS = 10;
		unsigned int numThreads = std::thread::hardware_concurrency();
		std::thread threads[MAX_THREADS];

		if (numThreads > MAX_THREADS)
			numThreads = MAX_THREADS;
		if (numThreads == 0)
			numThreads = DEFAULT_THREADS;
	}


	//---------------------------------------------------------------------------------------------------------------------

	// Render scene to current render target
	void EngineTest::SceneRender()
	{
		
	}


	//---------------------------------------------------------------------------------------------------------------------

	// Execute this app. Call this function after construction.
	// Returns: Exit-code - EXIT_SUCCESS or EXIT_FAILURE.
	int EngineTest::Run()
	{
		// Size and show window and attach a DirectX engine to it
		mWindowSize = {1600, 960};
		SDL_SetWindowSize(mWindow, mWindowSize.x(), mWindowSize.y());
		SDL_SetWindowPosition(mWindow, 32, 48);
		SDL_ShowWindow(mWindow);
		mEngine = std::make_unique<EngineDX>(mWindow);

		if (!SceneSetup())
		{
			return EXIT_FAILURE;
		}

		mTimer.Reset();
		mTimer.Start();

		// Game loop
		while (!PollEvents() && !mInput.KeyHit(SDLK_ESCAPE)) // Quit if window closed or escape pressed
		{
			SceneUpdate();
			mEngine->ClearBackBuffer();
			SceneRender();
			mEngine->Present();

			UpdateFrameTime();
			std::string title = "FPS: " + std::to_string(1.0f / mAverageFrameTime);
			SDL_SetWindowTitle(mWindow, title.c_str());
		}

		return EXIT_SUCCESS;
	}


	//---------------------------------------------------------------------------------------------------------------------
	// Support functions
	//---------------------------------------------------------------------------------------------------------------------

	// Update frame timing - call once every render loop. Use mFrameTime or mAverageFrameTime for timing
	void EngineTest::UpdateFrameTime()
	{
		mFrameTime = mTimer.GetLapTime();

		// Calculate averaged frame time over a period of time
		mSumFrameTimes += mFrameTime;
		mNumFrameTimes++;
		if (mSumFrameTimes > mFPSUpdateTime)
		{
			mAverageFrameTime = mSumFrameTimes / mNumFrameTimes;
			mSumFrameTimes = 0.0f;
			mNumFrameTimes = 0;
		}
	}


	// Poll SDL window/input events - call once every render loop. Returns true if application should close
	bool EngineTest::PollEvents()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				return true;
			}
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
			{
				mInput.KeyEvent(e.key);
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
			{
				mInput.MouseButtonEvent(e.button);
			}
			else if (e.type == SDL_MOUSEMOTION)
			{
				mInput.MouseMotionEvent(e.motion);
			}
			else if (e.type == SDL_MOUSEWHEEL)
			{
				mInput.MouseWheelEvent(e.wheel);
			}
			else if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED)
			{
				mInput.UpdateAttachedControllers();
			}
		}
		return false;
	}


	// Callback to immediately deal with certain events
	// Returns: 1 to store event in queue as normal (it can be polled), 0 to drop event
	// Note: Currently unused placeholder for mobile platforms
	int EngineTest::EventFilter(void*, SDL_Event*)
	{
		return 1; // Don't drop any events
	}
} // namespaces
