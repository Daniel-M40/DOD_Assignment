// Declares EngineTest, a class to test the sprite engine API

#ifndef MSC_ENGINE_TEST_H_INCLUDED
#define MSC_ENGINE_TEST_H_INCLUDED

#include "sim_config.h"
#include "platform/sdl_init.h"
#include "circle_gpu.h"
#include <sdl.h>
#include "platform/input.h"
#include "platform/timer.h"
#include "platform/window.h"

#include <Eigen/Core>

#include "thread_pool.h"
#include "spatial_hash.h"
using Eigen::Vector2f;
using Eigen::Vector2i;

namespace msc
{
	class EngineDX;

	enum class ENodeType
	{
		Attractor,
		Repulsor,
	};

	class EngineTest
	{
	public:
		EngineTest();
		~EngineTest();

		EngineTest(const EngineTest&) = delete;
		EngineTest(EngineTest&&) = delete;
		EngineTest& operator=(const EngineTest&) = delete;
		EngineTest& operator=(EngineTest&&) = delete;

		int Run();
		bool SceneSetup();
		void SceneRender();
		void SceneUpdate();

	private:
		bool PollEvents();
		void UpdateFrameTime();
		static int EventFilter(void* userdata, SDL_Event* event);

	private:
		// Platform specifics
#ifdef VISUALISATION_ENABLED
		platform::SDL mSDL;
		platform::Window mWindow;

		std::vector<CircleGPU> gpuCircleData;
		std::vector<CircleGPU> gpuNodeData;
		std::unique_ptr<EngineDX> mEngine;
		Vector2i mWindowSize;
#endif

		platform::Input mInput;
		platform::Timer mTimer;

		//-----------------------------------------------------
		// Thread Pool
		//-----------------------------------------------------
#ifdef THREAD_POOL_ENABLED
		ThreadPool mThreadPool;
		int mNumThreads = std::thread::hardware_concurrency();
#endif

		//-----------------------------------------------------
		// Spatial Hash
		//-----------------------------------------------------
#ifdef SPATIAL_HASH_ENABLED
#ifdef ENABLE_3D
		SpatialHash<3> mSpatialHash;
#else
		SpatialHash<2> mSpatialHash;
#endif
#endif

		//-----------------------------------------------------
		// Circle SoA data
		//-----------------------------------------------------
		std::vector<float>			mPosX;
		std::vector<float>			mPosY;
#ifdef ENABLE_3D
		std::vector<float>			mPosZ;
		std::vector<float>			mVelZ;
#endif
		std::vector<float>			mVelX;
		std::vector<float>			mVelY;
		std::vector<float>			mRadii;
		std::vector<float>			mColR;
		std::vector<float>			mColG;
		std::vector<float>			mColB;
		uint32_t					mActiveCount = 0;
		uint32_t					mNodeActiveCount = 0;

		// Death system data — only allocated when death is enabled to save cache
		std::vector<int32_t>		mHP;
		std::vector<uint8_t>		mAlive;
		// Names only allocated when death is enabled (used for debug logging)
		std::vector<std::string>	mNames;

		//-----------------------------------------------------
		// Nodes SoA data
		//-----------------------------------------------------
		std::vector<float>		mNodePosX;
		std::vector<float>		mNodePosY;
#ifdef ENABLE_3D
		std::vector<float>			mNodePosZ;
		std::vector<float>			mNodeVelZ;
#endif
		std::vector<float>		mNodeVelX;
		std::vector<float>		mNodeVelY;
		std::vector<float>		mNodeColR;
		std::vector<float>		mNodeColG;
		std::vector<float>		mNodeColB;
		std::vector<float>		mNodeRadii;
		std::vector<ENodeType>	mNodeType;
		std::vector<float>		mNodePeriod;
		std::vector<float>		mNodeTimer;
		std::vector<uint32_t>	mActiveNodes;

		// Frame timing
		float mFrameTime = 0.01f;
		float mSumFrameTimes = 0.0f;
		uint32_t mNumFrameTimes = 0;
		float mAverageFrameTime = 0.01f;
		float mAverageFPS = 1 / mAverageFrameTime;
		const float mFPSUpdateTime = 0.5f;


		void UpdateCircles(uint32_t start, uint32_t end);
		void NodeActionResolution(uint32_t nodeIndex, uint32_t circleIndex, float radiusSqrd);
		void CircleCollisionResolution(uint32_t i, uint32_t j);

		float GetZ(uint32_t i) const
		{
#ifdef ENABLE_3D
			return mPosZ[i];
#else
			return 0.0f;
#endif
		}

		float GetNodeZ(uint32_t i) const
		{
#ifdef ENABLE_3D
			return mNodePosZ[i];
#else
			return 0.0f;
#endif
		}
	};
} // namespaces

#endif // Header guard