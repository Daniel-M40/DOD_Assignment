// Implements EngineTest, a class to test the sprite engine API
#include "engine_test.h"

#include <sdl.h>
#include "sim_config.h"
#include "utility.h"
#include "render/engine_dx.h"
#include <iostream>

namespace msc
{

	// Constructor initialises SDL to receive input/window events and also creates a (hidden) window
#ifdef VISUALISATION_ENABLED
	EngineTest::EngineTest() : mSDL(SDL_INIT_EVENTS), mWindow("DOD Assignment", false)
	{
		SDL_SetEventFilter(EventFilter, this);
	}

#else
	EngineTest::EngineTest()
	{

	}
#endif

	EngineTest::~EngineTest()
	{
	}


	#pragma region Scene Lifecycle

	// Load and pre-process game resources. Returns true on success
	bool EngineTest::SceneSetup()
	{
		mActiveCount = SimConfig::NUM_CIRCLES;
		mNodeActiveCount = SimConfig::NUM_CIRCLES / 2000;

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

		mNodePosX.resize(mNodeActiveCount);
		mNodePosY.resize(mNodeActiveCount);
		mNodeVelX.resize(mNodeActiveCount);
		mNodeVelY.resize(mNodeActiveCount);
		mNodeRadii.resize(mNodeActiveCount);
		mNodeColR.resize(mNodeActiveCount);
		mNodeColG.resize(mNodeActiveCount);
		mNodeColB.resize(mNodeActiveCount);
		mNodeType.resize(mNodeActiveCount);
		mNodePeriod.resize(mNodeActiveCount);
		mNodeTimer.resize(mNodeActiveCount);

#ifdef VISUALISATION_ENABLED
		gpuCircleData.resize(mActiveCount);
		gpuNodeData.resize(mNodeActiveCount);
#endif

		float worldX = static_cast<float>(SimConfig::WORLD_SIZE_X);
		float worldY = static_cast<float>(SimConfig::WORLD_SIZE_Y);
		float minVel = -SimConfig::CIRCLE_MAX_VELOCITY;
		float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;

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

		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			mNodePosX[i] = RandomInRange(-worldX, worldX);
			mNodePosY[i] = RandomInRange(-worldY, worldY);

			if (SimConfig::MOVING_NODES)
			{
				mNodeVelX[i] = RandomInRange(minVel, maxVel);
				mNodeVelY[i] = RandomInRange(minVel, maxVel);
			}

			mNodeRadii[i] = RandomInRange(SimConfig::NODE_MIN_RADIUS, SimConfig::NODE_MAX_RADIUS);
			mNodeType[i] = static_cast<ENodeType>(RandomInRange(0, 1));
			mNodePeriod[i] = RandomInRange(SimConfig::NODE_MIN_TIME, SimConfig::NODE_MAX_TIME);
			mNodeTimer[i] = RandomInRange(0.f, mNodePeriod[i]);

			mNodeColR[i] = (mNodeType[i] == ENodeType::Attractor) ? 0.0f : 1.0f;
			mNodeColG[i] = (mNodeType[i] == ENodeType::Attractor) ? 1.0f : 0.0f;
			mNodeColB[i] = (mNodeType[i] == ENodeType::Attractor) ? 0.0f : 0.0f;
		}


		return true;
	}

	// Main scene update code. Called every frame, uses mFrameTime member for timing
	void EngineTest::SceneUpdate()
	{
		#ifdef THREAD_POOL_ENABLED

		uint32_t chunk = mActiveCount / mNumThreads;
		for (int t = 0; t < mNumThreads; ++t)
		{
			uint32_t start = t * chunk;
			uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + chunk;
			mThreadPool.Submit([this, start, end] { UpdateCircles(start, end); });
		}

		mThreadPool.WaitAll();

		#else

		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			if (SimConfig::ENABLE_WALLS)
			{
				if (mPosX[i] >= SimConfig::WORLD_SIZE_X || mPosX[i] <= -SimConfig::WORLD_SIZE_X)
				{
					mVelX[i] = -mVelX[i];
				}

				if (mPosY[i] >= SimConfig::WORLD_SIZE_Y || mPosY[i] <= -SimConfig::WORLD_SIZE_Y)
				{
					mVelY[i] = -mVelY[i];
				}
			}

			mPosX[i] += mVelX[i] * mFrameTime;
			mPosY[i] += mVelY[i] * mFrameTime;
		}

		#endif




		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			mNodeTimer[i] -= mFrameTime;

			if (mNodeTimer[i] <= 0.f)
			{
				// reset timer and perform action
				mNodeTimer[i] += mNodePeriod[i];

				float radiusSqrd = mNodeRadii[i] * mNodeRadii[i];

				for (uint32_t j = 0; j < mActiveCount; ++j)
				{
					float dx = mPosX[j] - mNodePosX[i];
					float dy = mPosY[j] - mNodePosY[i];
					float distSqrd = dx * dx + dy * dy;

					if (distSqrd < radiusSqrd && distSqrd > 0.0001f)
					{
						float dist = sqrt(distSqrd);
						float sign = (mNodeType[i] == ENodeType::Attractor) ? -1.0f : 1.0f;
						float strength = sign * SimConfig::IMPULSE_NODE_STRENGTH / dist;

						mVelX[j] += (dx / dist) * strength;
						mVelY[j] += (dy / dist) * strength;

						mHP[j] -= 10;
#ifdef VISUALISATION_ENABLED
#else
						float simTime = mTimer.GetTime();
						//std::cout << "Time: " << simTime << "s | " << mNames[j] << " | HP: " << mHP[j] << std::endl;
#endif

					}
				}
			}
		}
	}

	// Render scene to current render target
	void EngineTest::SceneRender()
	{
		#ifdef VISUALISATION_ENABLED

		//CIRCLE RENDER PASS
		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			gpuCircleData[i].posX = mPosX[i];
			gpuCircleData[i].posY = mPosY[i];
			gpuCircleData[i].posZ = 0.5f;
			gpuCircleData[i].radius = mRadii[i];
			gpuCircleData[i].colR = mColR[i];
			gpuCircleData[i].colG = mColG[i];
			gpuCircleData[i].colB = mColB[i];
			gpuCircleData[i].padding = 0.0f;
		}
		mEngine->Render(gpuCircleData.data(), mActiveCount);

		//NODE RENDER PASS
		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			gpuNodeData[i].posX = mNodePosX[i];
			gpuNodeData[i].posY = mNodePosY[i];
			gpuNodeData[i].posZ = 0.5f;
			gpuNodeData[i].radius = mNodeRadii[i];
			gpuNodeData[i].colR = mNodeColR[i];
			gpuNodeData[i].colG = mNodeColG[i];
			gpuNodeData[i].colB = mNodeColB[i];
			gpuNodeData[i].padding = 1.0f;
		}
		mEngine->Render(gpuNodeData.data(), mNodeActiveCount);

		#endif
	}

	int EngineTest::Run()
	{
		#ifdef VISUALISATION_ENABLED
		mWindowSize = {1600, 960};
		SDL_SetWindowSize(mWindow, mWindowSize.x(), mWindowSize.y());
		SDL_SetWindowPosition(mWindow, 32, 48);
		SDL_ShowWindow(mWindow);
		mEngine = std::make_unique<EngineDX>(mWindow);
		#endif

		if (!SceneSetup())
		{
			return EXIT_FAILURE;
		}

		#ifdef VISUALISATION_ENABLED
		mEngine->SetRenderMode(RenderMode::Cutout);
	#endif

		mTimer.Reset();
		mTimer.Start();

		while (!PollEvents() && !mInput.KeyHit(SDLK_ESCAPE))
		{
			SceneUpdate();
			#ifdef VISUALISATION_ENABLED
			mEngine->ClearBackBuffer();
			SceneRender();
			mEngine->Present();
			#endif

			UpdateFrameTime();

#ifdef VISUALISATION_ENABLED
			std::string title = "FPS: " + std::to_string(1.0f / mAverageFrameTime);
			SDL_SetWindowTitle(mWindow, title.c_str());
#else
			static float printTimer = 0.0f;
			printTimer += mFrameTime;
			if (printTimer >= 1.0f)
			{
				std::cout << "Circles: " << mActiveCount
					<< " | Avg Frame: " << (mAverageFrameTime * 1000.0f) << "ms"
					<< " | FPS: " << (1.0f / mAverageFrameTime) << std::endl;
				printTimer = 0.0f;
			}
#endif
		}

		return EXIT_SUCCESS;
	}

	#pragma endregion

	#pragma region Circle and Node Update

	void EngineTest::UpdateCircles(uint32_t start, uint32_t end)
	{
		for (uint32_t i = start; i < end; ++i)
		{
			if (SimConfig::ENABLE_WALLS)
			{
				if (mPosX[i] >= SimConfig::WORLD_SIZE_X || mPosX[i] <= -SimConfig::WORLD_SIZE_X)
				{
					mVelX[i] = -mVelX[i];
				}

				if (mPosY[i] >= SimConfig::WORLD_SIZE_Y || mPosY[i] <= -SimConfig::WORLD_SIZE_Y)
				{
					mVelY[i] = -mVelY[i];
				}
			}

			mPosX[i] += mVelX[i] * mFrameTime;
			mPosY[i] += mVelY[i] * mFrameTime;
		}
	}

	#pragma endregion

	#pragma region Event Handling

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

	#pragma endregion

} // namespaces
