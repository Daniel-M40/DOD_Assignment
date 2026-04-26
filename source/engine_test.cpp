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

		// Only allocate death-system arrays when the feature is enabled.
		// When disabled this saves ~40+ bytes per circle of cache pressure
		// (4 bytes HP + 1 byte alive + ~32 bytes string overhead per circle).
		if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
		{
			mHP.resize(mActiveCount);
			mAlive.resize(mActiveCount, 1);
			mNames.resize(mActiveCount);
		}

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

			if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
			{
				mHP[i] = static_cast<int32_t>(SimConfig::CIRCLE_MAX_HEALTH);
				mNames[i] = "Circle_" + std::to_string(i);
			}

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
		// --- Phase 1: Update circle positions (parallelised) ---
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

		// --- Phase 2: Update node positions (parallelised) ---
		if (SimConfig::MOVING_NODES)
		{
#ifdef THREAD_POOL_ENABLED
			uint32_t nodeChunk = mNodeActiveCount / mNumThreads;
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t nStart = t * nodeChunk;
				uint32_t nEnd = (t == mNumThreads - 1) ? mNodeActiveCount : nStart + nodeChunk;
				mThreadPool.Submit([this, nStart, nEnd]
					{
						for (uint32_t i = nStart; i < nEnd; ++i)
						{
							if (SimConfig::ENABLE_WALLS)
							{
								if (mNodePosX[i] >= SimConfig::WORLD_SIZE_X || mNodePosX[i] <= -SimConfig::WORLD_SIZE_X)
									mNodeVelX[i] = -mNodeVelX[i];
								if (mNodePosY[i] >= SimConfig::WORLD_SIZE_Y || mNodePosY[i] <= -SimConfig::WORLD_SIZE_Y)
									mNodeVelY[i] = -mNodeVelY[i];
#ifdef ENABLE_3D
								if (mNodePosZ[i] >= SimConfig::WORLD_SIZE_Z || mNodePosZ[i] <= -SimConfig::WORLD_SIZE_Z)
									mNodeVelZ[i] = -mNodeVelZ[i];
#endif
							}
							mNodePosX[i] += mNodeVelX[i] * mFrameTime;
							mNodePosY[i] += mNodeVelY[i] * mFrameTime;
#ifdef ENABLE_3D
							mNodePosZ[i] += mNodeVelZ[i] * mFrameTime;
#endif
						}
					});
			}
			mThreadPool.WaitAll();
#else
			for (uint32_t i = 0; i < mNodeActiveCount; ++i)
			{
				if (SimConfig::ENABLE_WALLS)
				{
					if (mNodePosX[i] >= SimConfig::WORLD_SIZE_X || mNodePosX[i] <= -SimConfig::WORLD_SIZE_X)
						mNodeVelX[i] = -mNodeVelX[i];
					if (mNodePosY[i] >= SimConfig::WORLD_SIZE_Y || mNodePosY[i] <= -SimConfig::WORLD_SIZE_Y)
						mNodeVelY[i] = -mNodeVelY[i];
#ifdef ENABLE_3D
					if (mNodePosZ[i] >= SimConfig::WORLD_SIZE_Z || mNodePosZ[i] <= -SimConfig::WORLD_SIZE_Z)
						mNodeVelZ[i] = -mNodeVelZ[i];
#endif
				}
				mNodePosX[i] += mNodeVelX[i] * mFrameTime;
				mNodePosY[i] += mNodeVelY[i] * mFrameTime;
#ifdef ENABLE_3D
				mNodePosZ[i] += mNodeVelZ[i] * mFrameTime;
#endif
			}
#endif
		}

		// --- Phase 3: Rebuild spatial hash (parallelised count + insert) ---
#ifdef SPATIAL_HASH_ENABLED
		mSpatialHash.Clear();

#ifdef THREAD_POOL_ENABLED
		// Parallel count: each thread counts its chunk
		uint32_t countChunk = mActiveCount / mNumThreads;
		for (int t = 0; t < mNumThreads; ++t)
		{
			uint32_t cStart = t * countChunk;
			uint32_t cEnd = (t == mNumThreads - 1) ? mActiveCount : cStart + countChunk;
			mThreadPool.Submit([this, cStart, cEnd]
				{
					for (uint32_t i = cStart; i < cEnd; ++i)
						mSpatialHash.CountAtomic(mPosX[i], mPosY[i], GetZ(i));
				});
		}
		mThreadPool.WaitAll();

		mSpatialHash.BuildOffsets();

		// Parallel insert: each thread scatters its chunk
		for (int t = 0; t < mNumThreads; ++t)
		{
			uint32_t cStart = t * countChunk;
			uint32_t cEnd = (t == mNumThreads - 1) ? mActiveCount : cStart + countChunk;
			mThreadPool.Submit([this, cStart, cEnd]
				{
					for (uint32_t i = cStart; i < cEnd; ++i)
						mSpatialHash.InsertAtomic(mPosX[i], mPosY[i], GetZ(i), i);
				});
		}
		mThreadPool.WaitAll();
#else
		for (uint32_t i = 0; i < mActiveCount; ++i)
			mSpatialHash.Count(mPosX[i], mPosY[i], GetZ(i));

		mSpatialHash.BuildOffsets();

		for (uint32_t i = 0; i < mActiveCount; ++i)
			mSpatialHash.Insert(mPosX[i], mPosY[i], GetZ(i), i);
#endif
#endif

		// --- Phase 4: Collect active nodes and process (parallelised) ---
		mActiveNodes.clear();
		for (uint32_t i = 0; i < mNodeActiveCount; ++i)
		{
			mNodeTimer[i] -= mFrameTime;
			if (mNodeTimer[i] <= 0.f)
			{
				mNodeTimer[i] += mNodePeriod[i];
				mActiveNodes.push_back(i);
			}
		}

		if (!mActiveNodes.empty())
		{
#ifdef THREAD_POOL_ENABLED
			uint32_t nodeCount = static_cast<uint32_t>(mActiveNodes.size());
			uint32_t nodeChunkSize = (std::max)(1u, nodeCount / static_cast<uint32_t>(mNumThreads));
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t nStart = t * nodeChunkSize;
				if (nStart >= nodeCount) break;
				uint32_t nEnd = (t == mNumThreads - 1) ? nodeCount : (std::min)(nStart + nodeChunkSize, nodeCount);
				mThreadPool.Submit([this, nStart, nEnd]
					{
						constexpr float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;

						// Raw pointers — avoids vector bounds checking in the hot loop
						const float* pPosX = mPosX.data();
						const float* pPosY = mPosY.data();
						float* pVelX = mVelX.data();
						float* pVelY = mVelY.data();

						// Only grab HP pointer if death system is active
						int32_t* pHP = nullptr;
						if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
							pHP = mHP.data();

						for (uint32_t n = nStart; n < nEnd; ++n)
						{
							uint32_t i = mActiveNodes[n];
							float radiusSqrd = mNodeRadii[i] * mNodeRadii[i];
							float nodePX = mNodePosX[i];
							float nodePY = mNodePosY[i];
							float sign = (mNodeType[i] == ENodeType::Attractor) ? -1.0f : 1.0f;
							float signStrength = sign * SimConfig::IMPULSE_NODE_STRENGTH;

#ifdef SPATIAL_HASH_ENABLED
							mSpatialHash.QueryBatch(nodePX, nodePY, 0.0f, mNodeRadii[i],
								[&](const uint32_t* indices, uint32_t count)
								{
									uint32_t e = 0;

									// Batch process 4 circles per iteration.
									// Fixed-size local arrays enable compiler auto-vectorisation
									// into SIMD instructions (SSE/AVX) in optimised builds.
									constexpr uint32_t BATCH = 4;
									for (; e + BATCH <= count; e += BATCH)
									{
										uint32_t idx[BATCH];
										for (uint32_t b = 0; b < BATCH; ++b)
											idx[b] = indices[e + b];

										// Gather positions
										float px[BATCH], py[BATCH];
										for (uint32_t b = 0; b < BATCH; ++b)
										{
											px[b] = pPosX[idx[b]];
											py[b] = pPosY[idx[b]];
										}

										// Distance squared
										float dx[BATCH], dy[BATCH], distSqrd[BATCH];
										for (uint32_t b = 0; b < BATCH; ++b)
										{
											dx[b] = px[b] - nodePX;
											dy[b] = py[b] - nodePY;
											distSqrd[b] = dx[b] * dx[b] + dy[b] * dy[b];
										}

										// Range check
										bool inRange[BATCH];
										bool anyInRange = false;
										for (uint32_t b = 0; b < BATCH; ++b)
										{
											inRange[b] = (distSqrd[b] < radiusSqrd && distSqrd[b] > 0.0001f);
											anyInRange = anyInRange || inRange[b];
										}

										if (!anyInRange) continue;

										// Force factor (0 for out-of-range, avoids branching)
										float factor[BATCH];
										for (uint32_t b = 0; b < BATCH; ++b)
											factor[b] = inRange[b] ? (signStrength / distSqrd[b]) : 0.0f;

										// Apply velocity change and clamp
										for (uint32_t b = 0; b < BATCH; ++b)
										{
											float vx = pVelX[idx[b]] + dx[b] * factor[b];
											float vy = pVelY[idx[b]] + dy[b] * factor[b];

											if (vx > maxVel)  vx = maxVel;
											if (vx < -maxVel) vx = -maxVel;
											if (vy > maxVel)  vy = maxVel;
											if (vy < -maxVel) vy = -maxVel;

											pVelX[idx[b]] = vx;
											pVelY[idx[b]] = vy;
										}

										// HP/death — only when enabled, completely compiled out otherwise
										if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
										{
											for (uint32_t b = 0; b < BATCH; ++b)
											{
												if (inRange[b])
												{
													pHP[idx[b]] -= 10;
													if (pHP[idx[b]] <= 0)
														mAlive[idx[b]] = 0;
												}
											}
										}
									}

									// Scalar tail
									for (; e < count; ++e)
									{
										uint32_t j = indices[e];
										float dx = pPosX[j] - nodePX;
										float dy = pPosY[j] - nodePY;
										float distSqrd = dx * dx + dy * dy;

										if (distSqrd < radiusSqrd && distSqrd > 0.0001f)
										{
											float factor = signStrength / distSqrd;
											pVelX[j] += dx * factor;
											pVelY[j] += dy * factor;

											if (pVelX[j] > maxVel)  pVelX[j] = maxVel;
											if (pVelX[j] < -maxVel) pVelX[j] = -maxVel;
											if (pVelY[j] > maxVel)  pVelY[j] = maxVel;
											if (pVelY[j] < -maxVel) pVelY[j] = -maxVel;

											if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
											{
												pHP[j] -= 10;
												if (pHP[j] <= 0)
													mAlive[j] = 0;
											}
										}
									}
								});
#else
							for (uint32_t j = 0; j < mActiveCount; ++j)
							{
								NodeActionResolution(i, j, radiusSqrd);
							}
#endif
						}
					});
			}
			mThreadPool.WaitAll();
#else
			for (uint32_t n = 0; n < mActiveNodes.size(); ++n)
			{
				uint32_t i = mActiveNodes[n];
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
#endif
		}

		// --- Phase 5: Circle-circle collision (parallelised) ---
		if (SimConfig::CIRCLE_COLLISION_ENABLED)
		{
#ifdef THREAD_POOL_ENABLED
			uint32_t collChunk = mActiveCount / mNumThreads;
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t start = t * collChunk;
				uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + collChunk;
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

		// --- Phase 6: Remove dead circles (swap-and-pop) ---
		if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
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
					mAlive[i] = mAlive[last];
					// Move string instead of copy — avoids heap allocation
					mNames[i] = std::move(mNames[last]);

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
		mWindowSize = { 1600, 960 };
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
					mVelX[i] = -mVelX[i];

				if (mPosY[i] >= SimConfig::WORLD_SIZE_Y || mPosY[i] <= -SimConfig::WORLD_SIZE_Y)
					mVelY[i] = -mVelY[i];

#ifdef ENABLE_3D
				if (mPosZ[i] >= SimConfig::WORLD_SIZE_Z || mPosZ[i] <= -SimConfig::WORLD_SIZE_Z)
					mVelZ[i] = -mVelZ[i];
#endif
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
			float sign = (mNodeType[nodeIndex] == ENodeType::Attractor) ? -1.0f : 1.0f;
			float factor = sign * SimConfig::IMPULSE_NODE_STRENGTH / distSqrd;

			mVelX[circleIndex] += dx * factor;
			mVelY[circleIndex] += dy * factor;
#ifdef ENABLE_3D
			mVelZ[circleIndex] += dz * factor;
#endif

			constexpr float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;
			mVelX[circleIndex] = (std::max)(-maxVel, (std::min)(maxVel, mVelX[circleIndex]));
			mVelY[circleIndex] = (std::max)(-maxVel, (std::min)(maxVel, mVelY[circleIndex]));
#ifdef ENABLE_3D
			mVelZ[circleIndex] = (std::max)(-maxVel, (std::min)(maxVel, mVelZ[circleIndex]));
#endif

			if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
			{
				mHP[circleIndex] -= 10;
				if (mHP[circleIndex] <= 0)
					mAlive[circleIndex] = 0;
			}
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

			float nx = dx * invDist;
			float ny = dy * invDist;

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

			float relVelN = (mVelX[j] - mVelX[i]) * nx + (mVelY[j] - mVelY[i]) * ny;

#ifdef ENABLE_3D
			relVelN += (mVelZ[j] - mVelZ[i]) * nz;
#endif

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