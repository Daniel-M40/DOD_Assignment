// DirectX code for a sprite engine

#ifndef MSC_ENGINE_DX_H_INCLUDED
#define MSC_ENGINE_DX_H_INCLUDED

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi.h>
#include <list>
#include <memory>
#include <stdint.h>
#include <vector>

#include <Eigen/Core>
struct CircleGPU;
using Eigen::Vector2i;

namespace msc
{
	//-------------------------------------------------------------------------------------------------
	// Related Types
	//-------------------------------------------------------------------------------------------------

	enum class RenderMode
	{
		Cutout,
		Additive,
		Multiplicative,
		AlphaBlending,
	};


	//-------------------------------------------------------------------------------------------------
	// Class declaration
	//-------------------------------------------------------------------------------------------------

	// Forward declaration of referenced classes
	namespace platform
	{
		class Window;
	};

	class ShaderDX;

	class EngineDX
	{
#ifdef _DEBUG
  // Test for leaks when object is destroyed, declared as first member so destructor is called last
  class LeakTest { public: ~LeakTest(); } mLeakTest;
#endif

		//-------------------------------------------------------------------------------------------------
		// Construction / Initialisation
		//-------------------------------------------------------------------------------------------------
	public:
		// Pass window to attach DirectX to, optionally switch into fullscreen (current desktop resolution)
		EngineDX(platform::Window& window, bool fullscreen = false);
		~EngineDX();

		// Prevent copy/move/assignment
		EngineDX(const EngineDX&) = delete;
		EngineDX(EngineDX&&) = delete;
		EngineDX& operator=(const EngineDX&) = delete;
		EngineDX& operator=(EngineDX&&) = delete;


		//-------------------------------------------------------------------------------------------------
		// Properties
		//-------------------------------------------------------------------------------------------------
	public:
		Vector2i BackBufferSize() const { return mBackBufferSize; }

		//-----------------------------------------------------------------------------------------C--------
		// Rendering
		//-------------------------------------------------------------------------------------------------
	public:
		// Clear the screen - should always do this every frame. even if planning to render every pixel
		// because some GPUs gain efficiency by flagging the screen area as cleared
		void ClearBackBuffer();

		// Select a blending mode for subsequent rendering
		void SetRenderMode(RenderMode renderMode);

		// Display back buffer, optionally request vertical synchronisation
		void Present(bool vSync = false);

		void Render(const CircleGPU* data, uint32_t count);


		//-------------------------------------------------------------------------------------------------
		// Private structures
		//-------------------------------------------------------------------------------------------------
	private:
		// Description of monitor attached to a graphics adapter, with a list of supported screen modes (resolution etc.)
		struct Monitor
		{
			CComPtr<IDXGIOutput> dxgi; // DX interface for working with this monitor
			DXGI_OUTPUT_DESC desc;
			std::vector<DXGI_MODE_DESC> modes;
		};

		// Description of a graphics adapter (GPU) in the system, with a list of attached monitors
		struct Adapter
		{
			CComPtr<IDXGIAdapter> dxgi; // DX interface for working with this adapter
			DXGI_ADAPTER_DESC desc;
			std::vector<Monitor> monitors;
		};


		//-------------------------------------------------------------------------------------------------
		// Support functions
		//-------------------------------------------------------------------------------------------------
	private:
		// Collect information about available graphics adapters, attached monitors and supported display modes
		void CollectAdapterInfo();

		// Get a mode to match a monitor's current settings. No way to read current format, so it is always
		// set to DXGI_FORMAT_R8G8B8A8_UNORM. Can get current, or desktop-preferred refresh rate (default)
		bool GetCurrentMonitorMode(const Monitor& monitor, DXGI_MODE_DESC* outDesc,
		                           bool usePreferredRefreshRate = true) const;

		// Prepare various pipeline state objects. No flexibility in the engine here, just various hard-coded
		// settings for states, vertex layout, shaders, samplers, topology...
		void CreatePipelineStateObjects();

#ifdef _DEBUG
  // Initialise Direct3D debugging messages and breakpoints, call after device creation
  void InitDebugging();
#endif


		//-------------------------------------------------------------------------------------------------
		// Window related members (including DXGI)
		//-------------------------------------------------------------------------------------------------
	private:
		CComPtr<IDXGIFactory> mDXGIFactory;
		// Access to DXGI (used to create swap chain, but also handles attached graphics cards, supported resolutions etc.)
		std::vector<Adapter> mAdapters; // Graphics adapters in system, first one has the primary monitor attached

		bool mIsFullscreen; // Display is fullscreen, or windowed
		DXGI_MODE_DESC mFullScreenModeDesc; // Screen mode (resolution, refresh rate etc.) to use for fullscreen display
		Vector2i mWindowedSize; // Size of client area when windowed


		//-------------------------------------------------------------------------------------------------
		// DirectX 11 core interfaces - device, context and swap chain
		//-------------------------------------------------------------------------------------------------
	private:
		HWND mHWnd; // Window handle

		// Device interfaces and settings
		CComPtr<ID3D11Device> mDevice;
		D3D_FEATURE_LEVEL mFeatureLevel;
		CComPtr<ID3D11DeviceContext> mContext;

		// Swap chain / back buffer interfaces
		CComPtr<IDXGISwapChain> mSwapChain;
		CComPtr<ID3D11Texture2D> mBackBufferTexture;
		CComPtr<ID3D11RenderTargetView> mBackBufferRTV;
		CComPtr<ID3D11Texture2D> mDepthStencil;
		CComPtr<ID3D11DepthStencilView> mDepthStencilView;
		Vector2i mBackBufferSize;
		// Doesn't have to match window size, but final render will be scaled before presentation if it doesn't


		//-------------------------------------------------------------------------------------------------
		// Pipeline state and persistent resources
		//-------------------------------------------------------------------------------------------------
	private:
		// Only one set of shaders used here - for drawing sprites
		std::unique_ptr<ShaderDX> mCircleVS;
		std::unique_ptr<ShaderDX> mCircleGS;
		std::unique_ptr<ShaderDX> mCirclePS;

		// Vertex layout - describes the data structure that will be passed to the vertex shader. In this
		// app we always pass SpriteRenderData structs (see sprite_set.h), so only one layout is needed
		CComPtr<ID3D11InputLayout> mSpriteVertexLayout;

		// Pipeline state settings - depth states, blend states and rasterisation states
		CComPtr<ID3D11DepthStencilState> mDepthDisable;
		CComPtr<ID3D11DepthStencilState> mDepthWritesDisable;
		CComPtr<ID3D11DepthStencilState> mDepthEnable;
		CComPtr<ID3D11RasterizerState> mCullNone;
		CComPtr<ID3D11BlendState> mBlendNone;
		CComPtr<ID3D11BlendState> mBlendAdditive;
		CComPtr<ID3D11BlendState> mBlendMultiplicative;
		CComPtr<ID3D11BlendState> mBlendAlpha;

		// Sampler state - how to sample pixels from textures. Using bilinear with no mip-maps for everything here
		CComPtr<ID3D11SamplerState> mBilinearNoMipsSampler;

		// Constant buffer - shader data that is only changed once per sprite-set
		// First, a structure to hold the data CPU-side, then a DirectX object for the GPU-side buffer
		__declspec(align(16))
		// Align data to match HLSL packing rules - this structure must match exactly the shader version
		struct
		{
			float backbufferSize[2];
			// Not using eigen math library types here as they might conflict with packing requrements
			float atlasSize[2];
		} mPerSpriteSetCBStruct;

		CComPtr<ID3D11Buffer> mPerSpriteSetCB;

		// Vertex buffer for circle sprites - created once and re-used every frame, updated with new data every frame
		CComPtr<ID3D11Buffer> mCirclesVertexBuffer; 
	};
} // namespaces

#endif // Header guard
