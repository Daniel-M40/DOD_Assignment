// Implements EngineTest, a class to test the sprite engine API
#include "engine_test.h"

#include <sdl.h>
#include "sim_config.h"
#include "utility.h"
#include "render/engine_dx.h"
#include <iostream>

namespace msc
{

	EngineTest::EngineTest() :
		mSDL(SDL_INIT_EVENTS),
		mWindow("DOD Assignment", false),
		mSpatialHash(SimConfig::CELL_SIZE, SimConfig::WORLD_SIZE_X, SimConfig::WORLD_SIZE_Y
#ifdef ENABLE_3D
			, SimConfig::WORLD_SIZE_Z
#endif
		)
	{
		if constexpr (SimConfig::VISUALISATION)
		{
			SDL_SetEventFilter(EventFilter, this);
		}
	}

	EngineTest::~EngineTest()
	{
	}


#pragma region Scene Lifecycle

	bool EngineTest::SceneSetup()
	{
		mActiveCount = SimConfig::NUM_CIRCLES;
		mNodeActiveCount = SimConfig::NUM_CIRCLES / 20;

		mCircleData.resize(mActiveCount);
		mRadii.resize(mActiveCount);
		mColR.resize(mActiveCount);
		mColG.resize(mActiveCount);
		mColB.resize(mActiveCount);

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

		if constexpr (SimConfig::VISUALISATION)
		{
			gpuCircleData.resize(mActiveCount);
			gpuNodeData.resize(mNodeActiveCount);
		}

		float worldX = static_cast<float>(SimConfig::WORLD_SIZE_X);
		float worldY = static_cast<float>(SimConfig::WORLD_SIZE_Y);

#ifdef ENABLE_3D
		float worldZ = static_cast<float>(SimConfig::WORLD_SIZE_Z);
#endif

		float minVel = -SimConfig::CIRCLE_MAX_VELOCITY;
		float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;

		for (uint32_t i = 0; i < mActiveCount; ++i)
		{
			mCircleData[i].posX = RandomInRange(-worldX, worldX);
			mCircleData[i].posY = RandomInRange(-worldY, worldY);
			mCircleData[i].velX = RandomInRange(minVel, maxVel);
			mCircleData[i].velY = RandomInRange(minVel, maxVel);
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

			if constexpr (SimConfig::MOVING_NODES)
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

	void EngineTest::SceneUpdate()
	{
		// --- Phase 1: Update circle positions ---
		if constexpr (SimConfig::THREADING)
		{
			uint32_t chunk = mActiveCount / mNumThreads;
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t start = t * chunk;
				uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + chunk;
				mThreadPool.Submit([this, start, end] { UpdateCircles(start, end); });
			}
			mThreadPool.WaitAll();
		}
		else
		{
			UpdateCircles(0, mActiveCount);
		}

		// --- Phase 2: Update node positions ---
		if constexpr (SimConfig::MOVING_NODES)
		{
			if constexpr (SimConfig::THREADING)
			{
				uint32_t nodeChunk = mNodeActiveCount / mNumThreads;
				for (int t = 0; t < mNumThreads; ++t)
				{
					uint32_t nStart = t * nodeChunk;
					uint32_t nEnd = (t == mNumThreads - 1) ? mNodeActiveCount : nStart + nodeChunk;
					mThreadPool.Submit([this, nStart, nEnd]
						{
							for (uint32_t i = nStart; i < nEnd; ++i)
							{
								if constexpr (SimConfig::ENABLE_WALLS)
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
			}
			else
			{
				for (uint32_t i = 0; i < mNodeActiveCount; ++i)
				{
					if constexpr (SimConfig::ENABLE_WALLS)
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
			}
		}

		// --- Phase 3: Rebuild spatial hash ---
		if constexpr (SimConfig::SPATIAL_HASHING)
		{
			mSpatialHash.Clear();

			if constexpr (SimConfig::THREADING)
			{
				uint32_t countChunk = mActiveCount / mNumThreads;
				for (int t = 0; t < mNumThreads; ++t)
				{
					uint32_t cStart = t * countChunk;
					uint32_t cEnd = (t == mNumThreads - 1) ? mActiveCount : cStart + countChunk;
					mThreadPool.Submit([this, cStart, cEnd]
						{
							for (uint32_t i = cStart; i < cEnd; ++i)
								mSpatialHash.CountAtomic(mCircleData[i].posX, mCircleData[i].posY, GetZ(i));
						});
				}
				mThreadPool.WaitAll();

				mSpatialHash.BuildOffsets();

				for (int t = 0; t < mNumThreads; ++t)
				{
					uint32_t cStart = t * countChunk;
					uint32_t cEnd = (t == mNumThreads - 1) ? mActiveCount : cStart + countChunk;
					mThreadPool.Submit([this, cStart, cEnd]
						{
							for (uint32_t i = cStart; i < cEnd; ++i)
								mSpatialHash.InsertAtomic(mCircleData[i].posX, mCircleData[i].posY, GetZ(i), i);
						});
				}
				mThreadPool.WaitAll();
			}
			else
			{
				for (uint32_t i = 0; i < mActiveCount; ++i)
					mSpatialHash.Count(mCircleData[i].posX, mCircleData[i].posY, GetZ(i));

				mSpatialHash.BuildOffsets();

				for (uint32_t i = 0; i < mActiveCount; ++i)
					mSpatialHash.Insert(mCircleData[i].posX, mCircleData[i].posY, GetZ(i), i);
			}
		}

		// --- Phase 4: Node interactions ---
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
			if constexpr (SimConfig::THREADING)
			{
				uint32_t nodeCount = static_cast<uint32_t>(mActiveNodes.size());
				uint32_t nodeChunkSize = (std::max)(1u, nodeCount / static_cast<uint32_t>(mNumThreads));
				for (int t = 0; t < mNumThreads; ++t)
				{
					uint32_t nStart = t * nodeChunkSize;
					if (nStart >= nodeCount) break;
					uint32_t nEnd = (t == mNumThreads - 1) ? nodeCount : (std::min)(nStart + nodeChunkSize, nodeCount);
					mThreadPool.Submit([this, nStart, nEnd]
						{
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

								if constexpr (SimConfig::SPATIAL_HASHING)
								{
									mSpatialHash.QueryBatch(nodePX, nodePY, 0.0f, mNodeRadii[i],
										[&](const uint32_t* indices, uint32_t count)
										{
											if (!SimConfig::NODES_ACTIVE) return;

											ProcessNodeBatch(indices, count, nodePX, nodePY,
												radiusSqrd, signStrength, nullptr, nullptr, nullptr, nullptr, pHP);
										});
								}
								else
								{
									if (!SimConfig::NODES_ACTIVE) return;

									for (uint32_t j = 0; j < mActiveCount; ++j)
										NodeActionResolution(i, j, radiusSqrd);
								}
							}
						});
				}
				mThreadPool.WaitAll();
			}
			else
			{
				for (uint32_t n = 0; n < mActiveNodes.size(); ++n)
				{
					uint32_t i = mActiveNodes[n];
					float radiusSqrd = mNodeRadii[i] * mNodeRadii[i];

					if constexpr (SimConfig::SPATIAL_HASHING)
					{
						mSpatialHash.Query(mNodePosX[i], mNodePosY[i], GetNodeZ(i), mNodeRadii[i], [&](uint32_t j)
							{
								if (!SimConfig::NODES_ACTIVE) return;

								NodeActionResolution(i, j, radiusSqrd);
							});
					}
					else
					{
						if (!SimConfig::NODES_ACTIVE) return;

						for (uint32_t j = 0; j < mActiveCount; ++j)
							NodeActionResolution(i, j, radiusSqrd);
					}
				}
			}
		}

		// --- Phase 5: Circle-circle collision ---
		if constexpr (SimConfig::CIRCLE_COLLISION_ENABLED)
		{
			if constexpr (SimConfig::THREADING)
			{
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

								if constexpr (SimConfig::SPATIAL_HASHING)
								{
									mSpatialHash.Query(mCircleData[i].posX, mCircleData[i].posY, GetZ(i), searchRadius, [&](uint32_t j)
										{
											if (j <= i) return;
											CircleCollisionResolution(i, j);
										});
								}
								else
								{
									for (uint32_t j = i + 1; j < mActiveCount; ++j)
										CircleCollisionResolution(i, j);
								}
							}
						});
				}
				mThreadPool.WaitAll();
			}
			else
			{
				for (uint32_t i = 0; i < mActiveCount; ++i)
				{
					float searchRadius = mRadii[i] * 2.0f;

					if constexpr (SimConfig::SPATIAL_HASHING)
					{
						mSpatialHash.Query(mCircleData[i].posX, mCircleData[i].posY, GetZ(i), searchRadius, [&](uint32_t j)
							{
								if (j <= i) return;
								CircleCollisionResolution(i, j);
							});
					}
					else
					{
						for (uint32_t j = i + 1; j < mActiveCount; ++j)
							CircleCollisionResolution(i, j);
					}
				}
			}
		}

		// --- Phase 6: Remove dead circles ---
		if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
		{
			for (uint32_t i = 0; i < mActiveCount;)
			{
				if (mAlive[i] == 0)
				{
					uint32_t last = mActiveCount - 1;
					// Single struct copy replaces 4 separate float copies
					mCircleData[i] = mCircleData[last];
					mRadii[i] = mRadii[last];
					mColR[i] = mColR[last];
					mColG[i] = mColG[last];
					mColB[i] = mColB[last];
					mHP[i] = mHP[last];
					mAlive[i] = mAlive[last];
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

	void EngineTest::SceneRender()
	{
		if constexpr (!SimConfig::VISUALISATION) return;

		// Circle render pass reads position from single CircleData stream
		if constexpr (SimConfig::THREADING)
		{
			uint32_t chunk = mActiveCount / mNumThreads;
			for (int t = 0; t < mNumThreads; ++t)
			{
				uint32_t start = t * chunk;
				uint32_t end = (t == mNumThreads - 1) ? mActiveCount : start + chunk;
				mThreadPool.Submit([this, start, end]
					{
						for (uint32_t i = start; i < end; ++i)
						{
							gpuCircleData[i].posX = mCircleData[i].posX;
							gpuCircleData[i].posY = mCircleData[i].posY;

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
		}
		else
		{
			for (uint32_t i = 0; i < mActiveCount; ++i)
			{
				gpuCircleData[i].posX = mCircleData[i].posX;
				gpuCircleData[i].posY = mCircleData[i].posY;

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
		}

		mEngine->Render(gpuCircleData.data(), mActiveCount);

		if constexpr (!SimConfig::NODE_VISUALISATION_ENABLED) return;

		// Node render pass
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
	}

	int EngineTest::Run()
	{
		if constexpr (SimConfig::VISUALISATION)
		{
			mWindowSize = { 1600, 960 };
			SDL_SetWindowSize(mWindow, mWindowSize.x(), mWindowSize.y());
			SDL_SetWindowPosition(mWindow, 32, 48);
			SDL_ShowWindow(mWindow);
			mEngine = std::make_unique<EngineDX>(mWindow);
		}

		if (!SceneSetup())
			return EXIT_FAILURE;

		if constexpr (SimConfig::VISUALISATION)
			mEngine->SetRenderMode(RenderMode::Cutout);

		mTimer.Reset();
		mTimer.Start();

		while (!PollEvents() && !mInput.KeyHit(SDLK_ESCAPE))
		{
			SceneUpdate();

			if constexpr (SimConfig::VISUALISATION)
			{
				mEngine->ClearBackBuffer();
				SceneRender();
				mEngine->Present();
			}

			UpdateFrameTime();

			if constexpr (SimConfig::VISUALISATION)
			{
				std::string title = "FPS: " + std::to_string(1.0f / mAverageFrameTime);
				SDL_SetWindowTitle(mWindow, title.c_str());
			}
			else
			{
				static float printTimer = 0.0f;
				printTimer += mFrameTime;
				if (printTimer >= 1.0f)
				{
					std::cout << "Circles: " << mActiveCount
						<< " | Avg Frame: " << (mAverageFrameTime * 1000.0f) << "ms"
						<< " | FPS: " << (1.0f / mAverageFrameTime) << std::endl;
					printTimer = 0.0f;
				}
			}
		}

		return EXIT_SUCCESS;
	}

#pragma endregion

#pragma region Circle and Node Update


	void EngineTest::UpdateCircles(uint32_t start, uint32_t end)
	{
		for (uint32_t i = start; i < end; ++i)
		{
			CircleData& m = mCircleData[i];

			if constexpr (SimConfig::ENABLE_WALLS)
			{
				if (m.posX >= SimConfig::WORLD_SIZE_X || m.posX <= -SimConfig::WORLD_SIZE_X)
					m.velX = -m.velX;

				if (m.posY >= SimConfig::WORLD_SIZE_Y || m.posY <= -SimConfig::WORLD_SIZE_Y)
					m.velY = -m.velY;

#ifdef ENABLE_3D
				if (mPosZ[i] >= SimConfig::WORLD_SIZE_Z || mPosZ[i] <= -SimConfig::WORLD_SIZE_Z)
					mVelZ[i] = -mVelZ[i];
#endif
			}

			m.posX += m.velX * mFrameTime;
			m.posY += m.velY * mFrameTime;

#ifdef ENABLE_3D
			mPosZ[i] += mVelZ[i] * mFrameTime;
#endif
		}
	}

	void EngineTest::NodeActionResolution(uint32_t nodeIndex, uint32_t circleIndex, float radiusSqrd)
	{
		float dx = mCircleData[circleIndex].posX - mNodePosX[nodeIndex];
		float dy = mCircleData[circleIndex].posY - mNodePosY[nodeIndex];

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

			mCircleData[circleIndex].velX += dx * factor;
			mCircleData[circleIndex].velY += dy * factor;
#ifdef ENABLE_3D
			mVelZ[circleIndex] += dz * factor;
#endif

			constexpr float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;
			mCircleData[circleIndex].velX = (std::max)(-maxVel, (std::min)(maxVel, mCircleData[circleIndex].velX));
			mCircleData[circleIndex].velY = (std::max)(-maxVel, (std::min)(maxVel, mCircleData[circleIndex].velY));
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

	void EngineTest::ProcessNodeBatch(const uint32_t* indices, uint32_t count,
		float nodePX, float nodePY, float radiusSqrd, float signStrength,
		const float* pPosX, const float* pPosY, float* pVelX, float* pVelY,
		int32_t* pHP)
	{
		constexpr float maxVel = SimConfig::CIRCLE_MAX_VELOCITY;
		uint32_t e = 0;

		constexpr uint32_t BATCH = 4;
		for (; e + BATCH <= count; e += BATCH)
		{
			uint32_t idx[BATCH];
			for (uint32_t b = 0; b < BATCH; ++b)
				idx[b] = indices[e + b];

			float px[BATCH], py[BATCH];
			for (uint32_t b = 0; b < BATCH; ++b)
			{
				px[b] = mCircleData[idx[b]].posX;
				py[b] = mCircleData[idx[b]].posY;
			}

			float dx[BATCH], dy[BATCH], distSqrd[BATCH];
			for (uint32_t b = 0; b < BATCH; ++b)
			{
				dx[b] = px[b] - nodePX;
				dy[b] = py[b] - nodePY;
				distSqrd[b] = dx[b] * dx[b] + dy[b] * dy[b];
			}

			bool inRange[BATCH];
			bool anyInRange = false;
			for (uint32_t b = 0; b < BATCH; ++b)
			{
				inRange[b] = (distSqrd[b] < radiusSqrd && distSqrd[b] > 0.0001f);
				anyInRange = anyInRange || inRange[b];
			}

			if (!anyInRange) continue;

			float factor[BATCH];
			for (uint32_t b = 0; b < BATCH; ++b)
				factor[b] = inRange[b] ? (signStrength / distSqrd[b]) : 0.0f;

			for (uint32_t b = 0; b < BATCH; ++b)
			{
				float vx = mCircleData[idx[b]].velX + dx[b] * factor[b];
				float vy = mCircleData[idx[b]].velY + dy[b] * factor[b];

				if (vx > maxVel)  vx = maxVel;
				if (vx < -maxVel) vx = -maxVel;
				if (vy > maxVel)  vy = maxVel;
				if (vy < -maxVel) vy = -maxVel;

				mCircleData[idx[b]].velX = vx;
				mCircleData[idx[b]].velY = vy;
			}

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

		for (; e < count; ++e)
		{
			uint32_t j = indices[e];
			float dx = mCircleData[j].posX - nodePX;
			float dy = mCircleData[j].posY - nodePY;
			float distSqrd = dx * dx + dy * dy;

			if (distSqrd < radiusSqrd && distSqrd > 0.0001f)
			{
				float factor = signStrength / distSqrd;
				mCircleData[j].velX += dx * factor;
				mCircleData[j].velY += dy * factor;

				if (mCircleData[j].velX > maxVel)  mCircleData[j].velX = maxVel;
				if (mCircleData[j].velX < -maxVel) mCircleData[j].velX = -maxVel;
				if (mCircleData[j].velY > maxVel)  mCircleData[j].velY = maxVel;
				if (mCircleData[j].velY < -maxVel) mCircleData[j].velY = -maxVel;

				if constexpr (SimConfig::CIRCLE_DEATH_ENABLED)
				{
					pHP[j] -= 10;
					if (pHP[j] <= 0)
						mAlive[j] = 0;
				}
			}
		}
	}

	void EngineTest::CircleCollisionResolution(uint32_t i, uint32_t j)
	{
		float dx = mCircleData[j].posX - mCircleData[i].posX;
		float dy = mCircleData[j].posY - mCircleData[i].posY;

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
			mCircleData[i].posX -= nx * halfOverlap;
			mCircleData[i].posY -= ny * halfOverlap;
			mCircleData[j].posX += nx * halfOverlap;
			mCircleData[j].posY += ny * halfOverlap;

#ifdef ENABLE_3D
			float nz = dz * invDist;
			mPosZ[i] -= nz * halfOverlap;
			mPosZ[j] += nz * halfOverlap;
#endif

			float relVelN = (mCircleData[j].velX - mCircleData[i].velX) * nx + (mCircleData[j].velY - mCircleData[i].velY) * ny;

#ifdef ENABLE_3D
			relVelN += (mVelZ[j] - mVelZ[i]) * nz;
#endif

			if (relVelN < 0.0f)
			{
				mCircleData[i].velX += relVelN * nx;
				mCircleData[i].velY += relVelN * ny;
				mCircleData[j].velX -= relVelN * nx;
				mCircleData[j].velY -= relVelN * ny;

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
				return true;

			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
				mInput.KeyEvent(e.key);
			else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
				mInput.MouseButtonEvent(e.button);
			else if (e.type == SDL_MOUSEMOTION)
				mInput.MouseMotionEvent(e.motion);
			else if (e.type == SDL_MOUSEWHEEL)
				mInput.MouseWheelEvent(e.wheel);
			else if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED)
				mInput.UpdateAttachedControllers();
		}
		return false;
	}

	int EngineTest::EventFilter(void*, SDL_Event*)
	{
		return 1;
	}

#pragma endregion

} // namespaces