// Implements EngineTest, a class to test the sprite engine API
#include "engine_test.h"
#include <math.h>
#include <sdl.h>
#include "sim_config.h"
#include "utility.h"
#include "render/engine_dx.h"

namespace msc
{
	SimConfig config = {};

	// Constructor initialises SDL to receive input/window events and also creates a (hidden) window
	EngineTest::EngineTest() : mSDL(SDL_INIT_EVENTS), mWindow("DOD Assignment", false)
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
		mActiveCount = config.numCircles;

		mPosX.resize(mActiveCount);
		mPosY.resize(mActiveCount);
		mVelX.resize(mActiveCount);
		mVelY.resize(mActiveCount);
		mRadii.resize(mActiveCount);
		mColR.resize(mActiveCount);
		mColG.resize(mActiveCount);
		mColB.resize(mActiveCount);
		mHP.resize(mActiveCount);
		mNames.resize(mActiveCount);

		float worldX = config.worldSize.x();
		float worldY = config.worldSize.y();
		float minVel = config.maxVelocity.x();
		float maxVel = config.maxVelocity.y();

		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			mPosX[i] = RandomInRange(-worldX, worldX);
			mPosY[i] = RandomInRange(-worldY, worldY);
			mVelX[i] = RandomInRange(minVel, maxVel);
			mVelY[i] = RandomInRange(minVel, maxVel);
			mRadii[i] = 1.0f;
			mColR[i] = RandomInRange(0.0f, 1.0f);
			mColG[i] = RandomInRange(0.0f, 1.0f);
			mColB[i] = RandomInRange(0.0f, 1.0f);
			mHP[i] = 100;
			mNames[i] = "Circle_" + std::to_string(i);
		}

		return true;
	}

	//---------------------------------------------------------------------------------------------------------------------

	// Main scene update code. Called every frame, uses mFrameTime member for timing
	void EngineTest::SceneUpdate()
	{
		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			mPosX[i] += mVelX[i] * mFrameTime;
			mPosY[i] += mVelY[i] * mFrameTime;
		}
	}

	//---------------------------------------------------------------------------------------------------------------------

	// Render scene to current render target
	void EngineTest::SceneRender()
	{
		mEngine->Render();
	}

	//---------------------------------------------------------------------------------------------------------------------

	int EngineTest::Run()
	{
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

		while (!PollEvents() && !mInput.KeyHit(SDLK_ESCAPE))
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

	void EngineTest::UpdateFrameTime()
	{
		mFrameTime = mTimer.GetLapTime();

		mSumFrameTimes += mFrameTime;
		mNumFrameTimes++;
		if (mSumFrameTimes > mFPSUpdateTime)
		{
			mAverageFrameTime = mSumFrameTimes / mNumFrameTimes;
			mSumFrameTimes = 0.0f;
			mNumFrameTimes = 0;
		}
	}

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

	int EngineTest::EventFilter(void*, SDL_Event*)
	{
		return 1;
	}
} // namespaces
