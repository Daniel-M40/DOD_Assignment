// Implements EngineTest, a class to test the sprite engine API
#include "engine_test.h"

#include <sdl.h>
#include "sim_config.h"
#include "utility.h"
#include "render/engine_dx.h"
#include <iostream>

namespace msc
{

#ifdef VISUALISATION_ENABLED
	EngineTest::EngineTest() : mSDL(SDL_INIT_EVENTS), mWindow("DOD Assignment", false),
		mSpatialHash(SimConfig::CELL_SIZE, SimConfig::WORLD_SIZE_X, SimConfig::WORLD_SIZE_Y
#ifdef ENABLE_3D
			, SimConfig::WORLD_SIZE_Z
#endif
		)
	{
		SDL_SetEventFilter(EventFilter, this);
	}
#else
	EngineTest::EngineTest() :
		mSpatialHash(SimConfig::CELL_SIZE, SimConfig::WORLD_SIZE_X, SimConfig::WORLD_SIZE_Y
#ifdef ENABLE_3D
			, SimConfig::WORLD_SIZE_Z
#endif
		)
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
		mNodeActiveCount = SimConfig::NUM_CIRCLES / 20;

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
		mAlive.resize(mActiveCount, 1);

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

#ifdef ENABLE_3D
		mPosZ.resize(mActiveCount);
		mVelZ.resize(mActiveCount);
		mNodePosZ.resize(mNodeActiveCount);
		mNodeVelZ.resize(mNodeActiveCount);
#endif


#ifdef VISUALISATION_ENABLED
		gpuCircleData.resize(mActiveCount);
		gpuNodeData.resize(mNodeActiveCount);
#endif

		float worldX = static_cast<float>(SimConfig::WORLD_SIZE_X);
		float worldY = static_cast<float>(SimConfig::WORLD_SIZE_Y);

#ifdef ENABLE_3D
		float worldZ = static_cast<float>(SimConfig::WORLD_SIZE_Z);
#endif

		float minVel = -SimConfig::CIRCLE_MAX_VELOCITY;
		float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;

		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			mPosX[i] = RandomInRange(-worldX, worldX);
			mPosY[i] = RandomInRange(-worldY, worldY);
			mVelX[i] = RandomInRange(minVel, maxVel);
			mVelY[i] = RandomInRange(minVel, maxVel);
			mRadii[i] = SimConfig::CIRCLE_RADIUS;
			mColR[i] = RandomInRange(0.0f, 1.0f);
			mColG[i] = RandomInRange(0.0f, 1.0f);
			mColB[i] = RandomInRange(0.0f, 1.0f);
			mHP[i] = SimConfig::CIRCLE_MAX_HEALTH;
			mNames[i] = "Circle_" + std::to_string(i);

#ifdef ENABLE_3D
			mPosZ[i] = RandomInRange(-worldZ, worldZ);
			mVelZ[i] = RandomInRange(minVel, maxVel);
#endif
		}

		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			mNodePosX[i] = RandomInRange(-worldX, worldX);
			mNodePosY[i] = RandomInRange(-worldY, worldY);

#ifdef ENABLE_3D
			mNodePosZ[i] = RandomInRange(-worldZ, worldZ);
#endif


			if (SimConfig::MOVING_NODES)
			{
				mNodeVelX[i] = RandomInRange(minVel, maxVel);
				mNodeVelY[i] = RandomInRange(minVel, maxVel);

#ifdef ENABLE_3D
				mNodeVelZ[i] = RandomInRange(minVel, maxVel);
#endif

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

		UpdateCircles(0, mActiveCount);

		#endif

		if (SimConfig::MOVING_NODES)
		{
			for (uint32_t i = 0; i < mNodeActiveCount; ++i)
			{
				if (SimConfig::ENABLE_WALLS)
				{
					if (mNodePosX[i] >= SimConfig::WORLD_SIZE_X || mNodePosX[i] <= -SimConfig::WORLD_SIZE_X)
					{
						mNodeVelX[i] = -mNodeVelX[i];
					}
					if (mNodePosY[i] >= SimConfig::WORLD_SIZE_Y || mNodePosY[i] <= -SimConfig::WORLD_SIZE_Y)
					{
						mNodeVelY[i] = -mNodeVelY[i];
					}
#ifdef ENABLE_3D
					if (mNodePosZ[i] >= SimConfig::WORLD_SIZE_Z || mNodePosZ[i] <= -SimConfig::WORLD_SIZE_Z)
					{
						mNodeVelZ[i] = -mNodeVelZ[i];
					}
#endif
				}

				mNodePosX[i] += mNodeVelX[i] * mFrameTime;
				mNodePosY[i] += mNodeVelY[i] * mFrameTime;
#ifdef ENABLE_3D
				mNodePosZ[i] += mNodeVelZ[i] * mFrameTime;
#endif
			}
		}

		// Spatial hash insert
#ifdef SPATIAL_HASH_ENABLED
		mSpatialHash.Clear();
		for (size_t i = 0; i < mActiveCount; i++)
		{
			mSpatialHash.Insert(mPosX[i], mPosY[i], GetZ(i), i);
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

#ifdef SPATIAL_HASH_ENABLED
				mSpatialHash.Query(mNodePosX[i], mNodePosY[i], GetNodeZ(i), mNodeRadii[i], [&](uint32_t j)
					{
						NodeActionResolution(i, j, radiusSqrd);
					});
#else
				for (uint32_t j = 0; j < mActiveCount; ++j)
				{
					NodeActionResolution(i, j, radiusSqrd);
				}
#endif
			}
		}

		if (SimConfig::CIRCLE_COLLISION_ENABLED)
		{
#ifdef THREAD_POOL_ENABLED
			uint32_t chunk = mActiveCount / mNumThreads;
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t start = t * chunk;
				uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + chunk;
				mThreadPool.Submit([this, start, end]
					{
						for (uint32_t i = start; i < end; ++i)
						{
							float searchRadius = mRadii[i] * 2.0f;
							mSpatialHash.Query(mPosX[i], mPosY[i], GetZ(i), searchRadius, [&](uint32_t j)
								{
									if (j <= i) return;
									CircleCollisionResolution(i, j);
								});
						}
					});
			}
			mThreadPool.WaitAll();
#else
			for (uint32_t i = 0; i < mActiveCount; ++i)
			{
				float searchRadius = mRadii[i] * 2.0f;
#ifdef SPATIAL_HASH_ENABLED
				mSpatialHash.Query(mPosX[i], mPosY[i], GetZ(i), searchRadius, [&](uint32_t j)
					{
						if (j <= i) return;
						CircleCollisionResolution(i, j);
					});
#else
				for (uint32_t j = i + 1; j < mActiveCount; ++j)
				{
					CircleCollisionResolution(i, j);
				}
#endif
			}
#endif
		}

		if (SimConfig::CIRCLE_DEATH_ENABLED)
		{
			for (uint32_t i = 0; i < mActiveCount;)
			{
				if (mAlive[i] == 0)
				{
					uint32_t last = mActiveCount - 1;
					mPosX[i] = mPosX[last];
					mPosY[i] = mPosY[last];
					mVelX[i] = mVelX[last];
					mVelY[i] = mVelY[last];
					mRadii[i] = mRadii[last];
					mColR[i] = mColR[last];
					mColG[i] = mColG[last];
					mColB[i] = mColB[last];
					mHP[i] = mHP[last];
					mNames[i] = mNames[last];
					mAlive[i] = mAlive[last];

#ifdef ENABLE_3D
					mPosZ[i] = mPosZ[last];
					mVelZ[i] = mVelZ[last];
#endif

					mActiveCount--;
				}
				else
				{
					++i;
				}
			}
		}
	}

	// Render scene to current render target
	void EngineTest::SceneRender()
	{
		#ifdef VISUALISATION_ENABLED

		//CIRCLE RENDER PASS
#ifdef THREAD_POOL_ENABLED
		uint32_t chunk = mActiveCount / mNumThreads;

		for (int t = 0; t < mNumThreads; ++t)
		{
			uint32_t start = t * chunk;
			uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + chunk;
			mThreadPool.Submit([this, start, end]
			{
				for (uint32_t i = start; i < end; ++i)
				{
					gpuCircleData[i].posX = mPosX[i];
					gpuCircleData[i].posY = mPosY[i];

#ifdef ENABLE_3D
					gpuCircleData[i].posZ = (mPosZ[i] + SimConfig::WORLD_SIZE_Z) / (SimConfig::WORLD_SIZE_Z * 2.0f);
#else
					gpuCircleData[i].posZ = 0.1f;
#endif

					gpuCircleData[i].radius = mRadii[i];
					gpuCircleData[i].colR = mColR[i];
					gpuCircleData[i].colG = mColG[i];
					gpuCircleData[i].colB = mColB[i];
					gpuCircleData[i].padding = 0.0f;
				}
			});
		}

		mThreadPool.WaitAll();

#else
		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			gpuCircleData[i].posX = mPosX[i];
			gpuCircleData[i].posY = mPosY[i];

#ifdef ENABLE_3D
			gpuCircleData[i].posZ = (mPosZ[i] + SimConfig::WORLD_SIZE_Z) / (SimConfig::WORLD_SIZE_Z * 2.0f);
#else
			gpuCircleData[i].posZ = 0.1f;
#endif

			gpuCircleData[i].radius = mRadii[i];
			gpuCircleData[i].colR = mColR[i];
			gpuCircleData[i].colG = mColG[i];
			gpuCircleData[i].colB = mColB[i];
			gpuCircleData[i].padding = 0.0f;
		}
#endif

		mEngine->Render(gpuCircleData.data(), mActiveCount);

		if (!SimConfig::NODE_VISUALISATION_ENABLED) return;

		//NODE RENDER PASS
		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			gpuNodeData[i].posX = mNodePosX[i];
			gpuNodeData[i].posY = mNodePosY[i];
#ifdef ENABLE_3D
			gpuNodeData[i].posZ = (mNodePosZ[i] + SimConfig::WORLD_SIZE_Z) / (SimConfig::WORLD_SIZE_Z * 2.0f);
#else
			gpuNodeData[i].posZ = 0.1f;
#endif
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

				#ifdef ENABLE_3D

				if (mPosZ[i] >= SimConfig::WORLD_SIZE_Z || mPosZ[i] <= -SimConfig::WORLD_SIZE_Z)
				{
					mVelZ[i] = -mVelZ[i];
				}

				#endif // ENABLE_3D
			}

			mPosX[i] += mVelX[i] * mFrameTime;
			mPosY[i] += mVelY[i] * mFrameTime;

#ifdef ENABLE_3D
			mPosZ[i] += mVelZ[i] * mFrameTime;
#endif
		}
	}

	void EngineTest::NodeActionResolution(uint32_t nodeIndex, uint32_t circleIndex, float radiusSqrd)
	{
		float dx = mPosX[circleIndex] - mNodePosX[nodeIndex];
		float dy = mPosY[circleIndex] - mNodePosY[nodeIndex];

#ifdef ENABLE_3D
		float dz = mPosZ[circleIndex] - mNodePosZ[nodeIndex];
		float distSqrd = dx * dx + dy * dy + dz * dz;
#else 
		float distSqrd = dx * dx + dy * dy;
#endif


		if (distSqrd < radiusSqrd && distSqrd > 0.0001f)
		{
			float dist = sqrt(distSqrd);
			float sign = (mNodeType[nodeIndex] == ENodeType::Attractor) ? -1.0f : 1.0f;
			float strength = sign * SimConfig::IMPULSE_NODE_STRENGTH / dist;

			mVelX[circleIndex] += (dx / dist) * strength;
			mVelY[circleIndex] += (dy / dist) * strength;
#ifdef ENABLE_3D
			mVelZ[circleIndex] += (dz / dist) * strength;
#endif

			// Clamp velocity
			float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;
			if (mVelX[circleIndex] > maxVel) mVelX[circleIndex] = maxVel;
			if (mVelX[circleIndex] < -maxVel) mVelX[circleIndex] = -maxVel;
			if (mVelY[circleIndex] > maxVel) mVelY[circleIndex] = maxVel;
			if (mVelY[circleIndex] < -maxVel) mVelY[circleIndex] = -maxVel;
#ifdef ENABLE_3D
			if (mVelZ[circleIndex] > maxVel) mVelZ[circleIndex] = maxVel;
			if (mVelZ[circleIndex] < -maxVel) mVelZ[circleIndex] = -maxVel;
#endif

			mHP[circleIndex] -= 10;
			if (SimConfig::CIRCLE_DEATH_ENABLED && mHP[circleIndex] <= 0)
			{
				mAlive[circleIndex] = 0;
			}
#ifdef VISUALISATION_ENABLED
#else
			float simTime = mTimer.GetTime();
			//std::cout << "Time: " << simTime << "s | " << mNames[circleIndex] << " | HP: " << mHP[circleIndex] << std::endl;
#endif

		}
	}


	void EngineTest::CircleCollisionResolution(uint32_t i, uint32_t j)
	{
		float dx = mPosX[j] - mPosX[i];
		float dy = mPosY[j] - mPosY[i];

#ifdef ENABLE_3D
		float dz = mPosZ[j] - mPosZ[i];
		float distSqrd = dx * dx + dy * dy + dz * dz;
#else
		float distSqrd = dx * dx + dy * dy;
#endif

		float sumRadii = mRadii[i] + mRadii[j];
		float sumRadiiSqrd = sumRadii * sumRadii;

		if (distSqrd < sumRadiiSqrd && distSqrd > 0.0001f)
		{
			float dist = sqrt(distSqrd);
			float invDist = 1.0f / dist;
			float overlap = sumRadii - dist;

			// Normalised collision axis
			float nx = dx * invDist;
			float ny = dy * invDist;

			// Push apart by half overlap each
			float halfOverlap = overlap * 0.5f;
			mPosX[i] -= nx * halfOverlap;
			mPosY[i] -= ny * halfOverlap;
			mPosX[j] += nx * halfOverlap;
			mPosY[j] += ny * halfOverlap;

#ifdef ENABLE_3D
			float nz = dz * invDist;
			mPosZ[i] -= nz * halfOverlap;
			mPosZ[j] += nz * halfOverlap;
#endif

			// Relative velocity j minus i
			float relVelN = (mVelX[j] - mVelX[i]) * nx + (mVelY[j] - mVelY[i]) * ny;

#ifdef ENABLE_3D
			relVelN += (mVelZ[j] - mVelZ[i]) * nz;
#endif

			// Only resolve if approaching
			if (relVelN < 0.0f)
			{
				mVelX[i] += relVelN * nx;
				mVelY[i] += relVelN * ny;
				mVelX[j] -= relVelN * nx;
				mVelY[j] -= relVelN * ny;

#ifdef ENABLE_3D
				mVelZ[i] += relVelN * nz;
				mVelZ[j] -= relVelN * nz;
#endif
			}
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
