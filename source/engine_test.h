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
using Eigen::Vector2f;
using Eigen::Vector2i;

namespace msc
{
	// Forward declarations of classes that are only used by pointer or reference. Do this instead of including the header
	// files. Header files should include as little as possible to reduce compile times (and clarify dependencies).
	// However, there is an important and subtle rule regarding unique_ptr. There are several unique_ptr members in this
	// class that use these forward declarations, e.g. unique_ptr<ScrollPlane>. This is allowed as long as the containing
	// class (EngineTest here) has a constructor and destructor declared *but not defined* in this file. This class doesn't
	// actually need a destructor, but it is declared here and defined as {} in the cpp file.
	class EngineDX;

	enum class ENodeType
	{
		Attractor,
		Repulsor,
	};

	class EngineTest
	{
	public:
		//---------------------------------------------------------------------------------------------------------------------
		// Construction
		//---------------------------------------------------------------------------------------------------------------------

		// Both constructor and destructor declarations are required to be declared here, but defined in the cpp file due to use
		// of forward declarations with unique pointers (see comment above).
		EngineTest();
		~EngineTest();

		// Prevent copy/move/assignment
		EngineTest(const EngineTest&) = delete;
		EngineTest(EngineTest&&) = delete;
		EngineTest& operator=(const EngineTest&) = delete;
		EngineTest& operator=(EngineTest&&) = delete;


		//---------------------------------------------------------------------------------------------------------------------
		// Public Interface
		//---------------------------------------------------------------------------------------------------------------------

		// Application entry point. Call this function after construction.
		// Returns EXIT_SUCCESS or EXIT_FAILURE (same as main function)
		int Run();

		// Load and pre-process game resources. Returns true on success
		bool SceneSetup();

		// Render scene to current render target
		void SceneRender();

		// Main scene update code. Called every frame, uses mFrameTime member for timing
		void SceneUpdate();


		//---------------------------------------------------------------------------------------------------------------------
		// Private implementation
		//---------------------------------------------------------------------------------------------------------------------
	private:
		// Poll SDL window/input events - call once every render loop. Returns true if application should close
		bool PollEvents();

		// Update frame timing - call once every render loop. Use mFrameTime or mAverageFrameTime for timing
		void UpdateFrameTime();

		// Callback to immediately deal with certain events
		// Returns: 1 to store event in queue as normal (it can be polled), 0 to drop event
		// Note: Currently unused placeholder for mobile platforms
		static int EventFilter(void* userdata, SDL_Event* event);


		//---------------------------------------------------------------------------------------------------------------------
		// Data
		//---------------------------------------------------------------------------------------------------------------------
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

		//---------------------------------------------------------------------------------------------------------------------
		// Game Data
		//---------------------------------------------------------------------------------------------------------------------

		// Circle SoA data
		std::vector<float>			mPosX;
		std::vector<float>			mPosY;
		std::vector<float>			mVelX;
		std::vector<float>			mVelY;
		std::vector<float>			mRadii;
		std::vector<float>			mColR;
		std::vector<float>			mColG;
		std::vector<float>			mColB;
		std::vector<int32_t>		mHP;
		std::vector<std::string>	mNames;
		uint32_t					mActiveCount = 0;
		uint32_t					mNodeActiveCount = 0;



		// Nodes SoA data
		std::vector<float>		mNodePosX;
		std::vector<float>		mNodePosY;
		std::vector<float>		mNodeVelX;
		std::vector<float>		mNodeVelY;
		std::vector<float>		mNodeColR;
		std::vector<float>		mNodeColG;
		std::vector<float>		mNodeColB;
		std::vector<float>		mNodeRadii;
		std::vector<ENodeType>	mNodeType;
		std::vector<float>		mNodePeriod;
		std::vector<float>		mNodeTimer;

		// Frame timing - sensible initial values for first frame
		float mFrameTime					= 0.01f;	// Last frame's frametime
		float mSumFrameTimes				= 0.0f;		// Sum of recent frame times
		uint32_t mNumFrameTimes				= 0;		// Number of samples in the sum above
		float mAverageFrameTime				= 0.01f;	// Average frame time (over recent frames)
		float mAverageFPS					= 1 / mAverageFrameTime;
		const float mFPSUpdateTime = 0.5f;
	};
} // namespaces

#endif // Header guard
