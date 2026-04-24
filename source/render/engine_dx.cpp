// DirectX code for a sprite engine

#include "engine_dx.h"
#include <algorithm>
#include <stdexcept>
#include "shader_dx.h"
#include "utility_dx.h"
#include "../circle_gpu.h"
#include "../platform/window.h"
#include "../sim_config.h"

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif

namespace msc
{
	//-------------------------------------------------------------------------------------------------
	// Construction / Initialisation
	//-------------------------------------------------------------------------------------------------

	// Pass window to attach DirectX to, optionally switch into fullscreen (current desktop resolution)
	EngineDX::EngineDX(platform::Window& window, bool fullscreen /*= false*/)
	{
		HRESULT hr;

		//-----------------------------------------------
		// Query system

		// Access to DXGI for graphics adapters, attached monitors, supported resolutions etc.
		if (FAILED(CreateDXGIFactory( __uuidof(mDXGIFactory), (void**)(&mDXGIFactory) )))
		{
			throw std::runtime_error("DXGI initialisation failure");
		}

		// Get list of attached adapters (GPUs), and the monitors/resolutions supported for each
		CollectAdapterInfo();


		//-----------------------------------------------
		// DirectX initialisation

		// Direct3D device flags and list of feature levels that this app supports
		UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		constexpr D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
		// Want DirectX 11

		// Create Direct3D device, at this point just selecting the hardware to use
		// The initial NULL asks to use the default adapter (the default graphics card). There is no guarantee that the
		// default adapter is the most powerful one in a multi-GPU system (e.g. Intel Graphics + Dedicated GPU). Really
		// we should use the information collected from the EnumerateAdapters call a few lines above to select the best
		// GPU - but how might we determine which is the best GPU?
		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
		                       // Note: if you do select a specific adapter rather than NULL then must use D3D_DRIVER_TYPE_UNKNOWN
		                       featureLevels, _countof(featureLevels),
		                       D3D11_SDK_VERSION, &mDevice, &mFeatureLevel, &mContext);

		// There can be an error asking for D3D_FEATURE_LEVEL_11_1 on unsupported hardware, try again at 11.0
		// (This error seems to defeat the point of passing in a list of requested feature levels but it is a documented error so deal with it)
		if (hr == E_INVALIDARG)
		{
			hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
			                       &featureLevels[1], _countof(featureLevels) - 1,
			                       D3D11_SDK_VERSION, &mDevice, &mFeatureLevel, &mContext);
		}
		if (FAILED(hr))
		{
			throw std::runtime_error("DirectX initialisation failure - error creating device");
		}

#ifdef _DEBUG
  // Enable debugging of this device
  InitDebugging();
#endif


		//-----------------------------------------------
		// Get window / desktop info

		// Window handle from SDL
		SDL_SysWMinfo windowInfo;
		if (!window.PlatformInfo(&windowInfo))
		{
			throw std::runtime_error("DirectX initialisation failure - window is invalid");
		}
		mHWnd = windowInfo.info.win.window;

		// Size of targeted window (client area - within the window frame)
		RECT rc;
		GetClientRect(mHWnd, &rc);
		mWindowedSize = {rc.right - rc.left, rc.bottom - rc.top};
		mIsFullscreen = fullscreen;

		// Screen mode to match current mode of primary monitor
		DXGI_MODE_DESC currentMode;
		if (!GetCurrentMonitorMode(mAdapters[0].monitors[0], &currentMode, true))
		{
			throw std::runtime_error("DirectX initialisation failure - cannot query primary monitor");
		}


		//-----------------------------------------------
		// Create swap-chain (back and depth buffer)

		DXGI_SWAP_CHAIN_DESC swapDesc{};
		swapDesc.BufferDesc = currentMode; // For fullscreen, use same mode that monitor is already in
		if (!mIsFullscreen)
		{
			swapDesc.BufferDesc.Width = mWindowedSize.x();
			// For windowed, just change dimensions from monitor's initial mode
			swapDesc.BufferDesc.Height = mWindowedSize.y();
		}
		swapDesc.OutputWindow = mHWnd;
		swapDesc.Windowed = !mIsFullscreen;
		swapDesc.BufferCount = 1;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		if (FAILED(mDXGIFactory->CreateSwapChain(mDevice, &swapDesc, &mSwapChain)))
		{
			throw std::runtime_error("DirectX initialisation failure - error creating swap chain");
		}

		// Get back buffer that was actually created in swap chain
		if (FAILED(mSwapChain->GetBuffer( 0, __uuidof(mBackBufferTexture), (void**)&mBackBufferTexture )))
		{
			throw std::runtime_error("DirectX initialisation failure - error creating back buffer");
		}
		D3D11_TEXTURE2D_DESC desc;
		mDevice->CreateRenderTargetView(mBackBufferTexture, nullptr, &mBackBufferRTV);
		mBackBufferTexture->GetDesc(&desc);
		mBackBufferSize = {desc.Width, desc.Height};

		// Create a texture to use for a depth buffer
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = mBackBufferSize.x();
		textureDesc.Height = mBackBufferSize.y();
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;
		if (FAILED(mDevice->CreateTexture2D(&textureDesc, NULL, &mDepthStencil)))
		{
			throw std::runtime_error("DirectX initialisation failure - error creating depth buffer");
		}
		if (FAILED(mDevice->CreateDepthStencilView(mDepthStencil, NULL, &mDepthStencilView)))
		{
			throw std::runtime_error("DirectX initialisation failure - error creating depth stencil view");
		}

		// Set back buffer / depth stencil as render target
		mContext->OMSetRenderTargets(1, &mBackBufferRTV.p, mDepthStencilView);

		// Set viewport as entire back buffer
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<FLOAT>(mBackBufferSize.x());
		viewport.Height = static_cast<FLOAT>(mBackBufferSize.y());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		mContext->RSSetViewports(1, &viewport);

		// Create various pipline state objects
		CreatePipelineStateObjects();

		// Disable automatic alt-enter (and print-screen because DXGI_MWA_NO_ALT_ENTER doesn't work)
		mDXGIFactory->MakeWindowAssociation(mHWnd, DXGI_MWA_NO_WINDOW_CHANGES);
	}

	EngineDX::~EngineDX()
	{
	}


	//-------------------------------------------------------------------------------------------------
	// Adapters / Monitors
	//-------------------------------------------------------------------------------------------------

	// Define monitor screen mode ordering, broadly in descending order of values (quality)
	bool MonitorModeOrder(const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b)
	{
		// Most important - prefer unscaled modes. Want to get native resolution first
		if (a.Scaling == DXGI_MODE_SCALING_STRETCHED && b.Scaling != DXGI_MODE_SCALING_STRETCHED)
		{
			return false;
		}
		if (b.Scaling == DXGI_MODE_SCALING_STRETCHED && a.Scaling != DXGI_MODE_SCALING_STRETCHED)
		{
			return true;
		}

		// Prefer larger resolution area
		UINT areaA = a.Width * a.Height;
		UINT areaB = b.Width * b.Height;
		if (areaA < areaB)
		{
			return false;
		}
		if (areaA > areaB)
		{
			return true;
		}

		// Prefer higher bit depth
		uint32_t formatSizeA = SizeOfDXGIFormat(a.Format);
		uint32_t formatSizeB = SizeOfDXGIFormat(b.Format);
		if (formatSizeA < formatSizeB)
		{
			return false;
		}
		if (formatSizeA > formatSizeB)
		{
			return true;
		}

		// Prefer faster refresh rate
		double refreshRateA = static_cast<double>(a.RefreshRate.Numerator) / a.RefreshRate.Denominator;
		double refreshRateB = static_cast<double>(b.RefreshRate.Numerator) / b.RefreshRate.Denominator;
		if (refreshRateA < refreshRateB)
		{
			return false;
		}
		if (refreshRateA > refreshRateB)
		{
			return true;
		}

		if (a.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE && b.ScanlineOrdering !=
			DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE)
		{
			return true;
		}
		if (b.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE && a.ScanlineOrdering !=
			DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE)
		{
			return false;
		}

		return false;
	}


	// Collect information about available graphics adapters, attached monitors & supported display modes
	// First monitor on first adapter will be the primary monitor. Modes for monitors are sorted with the
	// function above so that the native resolution is likely to be the first entry in a monitor's mode list
	// Note: only looks for monitors supporting 8-bit integer RGBA colour (the usual). Other modes are not listed
	void EngineDX::CollectAdapterInfo()
	{
		// Errors from DirectX are not propagated, instead that particular adapter/monitor/mode is skipped
		HRESULT hr;

		// Enumerate adapters (graphics cards)
		mAdapters.clear();
		Adapter adapter;
		for (UINT a = 0; mDXGIFactory->EnumAdapters(a, &adapter.dxgi) == S_OK; ++a)
		{
			hr = adapter.dxgi->GetDesc(&adapter.desc);
			if (hr != S_OK)
			{
				continue; // Error, skip this adapter
			}

			// Enumerate monitors attached to each adapter (1st monitor on the 1st adapter is the primary)
			Monitor monitor;
			for (UINT m = 0; adapter.dxgi->EnumOutputs(m, &monitor.dxgi) == S_OK; ++m)
			{
				hr = monitor.dxgi->GetDesc(&monitor.desc);
				if (hr != S_OK)
				{
					continue; // Error, skip this monitor
				}

				// Get display modes supported by this monitor
				UINT numModes = 0;
				DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
				// Only look for standard colour depth - integer RGBA 8-bits each
				UINT flags = DXGI_ENUM_MODES_SCALING; // Will include non-native resolutions that require scaling
				do
				{
					if (monitor.dxgi->GetDisplayModeList(format, flags, &numModes, nullptr) != S_OK)
					{
						break;
					}
					monitor.modes.resize(numModes);
					hr = monitor.dxgi->GetDisplayModeList(format, flags, &numModes, monitor.modes.data());
				}
				while (hr == DXGI_ERROR_MORE_DATA);
				if (hr != S_OK)
				{
					monitor.modes.clear(); // List the monitor, but no modes on error
				}

				// Sort monitor modes, broadly in descending order of values (quality)
				std::sort(monitor.modes.begin(), monitor.modes.end(), MonitorModeOrder);

				adapter.monitors.push_back(monitor);
				monitor = {};
			}
			mAdapters.push_back(adapter);
			adapter = {};
		}
	}

	// Get a screen mode description to match a monitor's current settings. No way to read current format, so it 
	// is always set to DXGI_FORMAT_R8G8B8A8_UNORM. Can get current, or desktop-preferred refresh rate (default)
	bool EngineDX::GetCurrentMonitorMode(const Monitor& monitor, DXGI_MODE_DESC* outDesc,
	                                     bool usePreferredRefreshRate /*= true*/) const
	{
		// Get details about given monitor
		HDC hdcMonitor = CreateDCW(monitor.desc.DeviceName, monitor.desc.DeviceName, nullptr, nullptr);
		if (hdcMonitor == nullptr)
		{
			return false;
		}
		UINT monitorWidth = GetDeviceCaps(hdcMonitor, HORZRES);
		UINT monitorHeight = GetDeviceCaps(hdcMonitor, VERTRES);
		UINT refreshRate = GetDeviceCaps(hdcMonitor, VREFRESH);
		DeleteDC(hdcMonitor);

		DXGI_MODE_DESC searchDesc = {};
		searchDesc.Width = monitorWidth;
		searchDesc.Height = monitorHeight;
		bool useDefaultRefreshRate = (refreshRate == 0 || refreshRate == 1 || usePreferredRefreshRate);
		searchDesc.RefreshRate.Numerator = useDefaultRefreshRate ? 0 : refreshRate;
		searchDesc.RefreshRate.Denominator = useDefaultRefreshRate ? 0 : 1;
		searchDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Have to assume this is the format since no
		searchDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		searchDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		return (monitor.dxgi->FindClosestMatchingMode(&searchDesc, outDesc, nullptr) == S_OK);
	}


	//-------------------------------------------------------------------------------------------------
	// Pipeline state
	//-------------------------------------------------------------------------------------------------

	// DirectX "vertex layout" description of SpriteRenderData structure (from sprite_set.h) that will be
	// passed to the vertex shader. In a change from previous year's material the "semantics" are now
	// simply the variable names (index is only needed when the semantic/variable name ends in numbers)
	// Slot, Slot Class and Instance Step are not needed here and are just set to the defaults
	D3D11_INPUT_ELEMENT_DESC CircleDataLayout[] =
	{
		// Semantic     Index  Format                        Slot  Offset  Slot Class                    Instance Step
		{"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"radius",   0, DXGI_FORMAT_R32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"colour",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"padding",  0, DXGI_FORMAT_R32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	// Prepare various pipeline state objects. No flexibility in the engine here, just various hard-coded settings
	void EngineDX::CreatePipelineStateObjects()
	{
		// Only one set of vertex/geometry/pixel shaders used in this app. They convert sprite instances into
		// textured quads with appropriate UV offsetting into sprite atlas
		mCircleVS = std::make_unique<ShaderDX>(mDevice, ShaderType::Vertex, "circle_vs");
		mCircleGS = std::make_unique<ShaderDX>(mDevice, ShaderType::Geometry, "circle_gs");
		mCirclePS = std::make_unique<ShaderDX>(mDevice, ShaderType::Pixel, "circle_ps");


		// Create the vertex layout so the vertex shader knows what data to expect. This will convert the
		// CircleDataLayout structure above into a DirectX object we use at render time. Need to pass an
		// "example" vertex shader to validate the layout, which is why mCircleVS is used
		mDevice->CreateInputLayout(CircleDataLayout,
		                           sizeof(CircleDataLayout) / sizeof(CircleDataLayout[0]),
		                           mCircleVS->ByteCode(), mCircleVS->ByteCodeLength(), &mSpriteVertexLayout);
		mContext->IASetInputLayout(mSpriteVertexLayout);


		// Depth buffer states
		D3D11_DEPTH_STENCIL_DESC depthDesc{}; // Initialise all members to 0
		depthDesc.DepthEnable = false;
		depthDesc.StencilEnable = false;
		mDevice->CreateDepthStencilState(&depthDesc, &mDepthDisable);
		depthDesc.DepthEnable = true;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Important so we get painter's algorithm for same z
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		mDevice->CreateDepthStencilState(&depthDesc, &mDepthWritesDisable);
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		mDevice->CreateDepthStencilState(&depthDesc, &mDepthEnable);
		mContext->OMSetDepthStencilState(mDepthDisable, 0); // Default to no depth buffer - painter's algorithm

		// Rasterizer states
		D3D11_RASTERIZER_DESC rasterDesc{}; // Initialise all members to 0
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		mDevice->CreateRasterizerState(&rasterDesc, &mCullNone);
		mContext->RSSetState(mCullNone); // Default to no front/back culling - not needed for sprites

		// Blending states
		D3D11_BLEND_DESC blendDesc{}; // Initialise all members to 0
		blendDesc.RenderTarget[0].BlendEnable = false;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		mDevice->CreateBlendState(&blendDesc, &mBlendNone);
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		mDevice->CreateBlendState(&blendDesc, &mBlendAdditive);
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		mDevice->CreateBlendState(&blendDesc, &mBlendMultiplicative);
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		mDevice->CreateBlendState(&blendDesc, &mBlendAlpha);
		mContext->OMSetBlendState(mBlendNone, nullptr, 0xffffffff); // Default to no blending


		// Sampler state - how to sample pixels out of textures. Only using point-sampling filtering here
		D3D11_SAMPLER_DESC samplerDesc{}; // Initialise all members to 0
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // Point sampling, no mip-maps
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // Everything else is the defaults...
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		mDevice->CreateSamplerState(&samplerDesc, &mBilinearNoMipsSampler);
		mContext->PSSetSamplers(0, 1, &mBilinearNoMipsSampler.p);
		// Set bilinear sampler straight away on pixel shader for texture 0
		// Note: Where you are supposed to pass an array of pointers, CComPtr crashes unless you
		//       add the .p - only pass one pointer though, use raw pointers if you need an array of several

		// Create constant buffer - for shader data that is only changed once per sprite-set
		D3D11_BUFFER_DESC bufferDesc{}; // Initialise all members to 0
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.ByteWidth = sizeof(mPerFrameCBStruct);
		mDevice->CreateBuffer(&bufferDesc, nullptr, &mPerFrameCB);
		mContext->GSSetConstantBuffers(0, 1, &mPerFrameCB.p);
		mContext->PSSetConstantBuffers(0, 1, &mPerFrameCB.p);
		// Set constant buffer straight away on geometry shader buffer 0

		// All rendering will be points (converted to quads in the shaders)
		mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);


		// Create constant buffer - for shader data that is only changed once per sprite-set
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.ByteWidth = SimConfig::NUM_CIRCLES * sizeof(CircleGPU);
		mDevice->CreateBuffer(&bufferDesc, nullptr, &mCirclesVertexBuffer);
	}


	//-----------------------------------------------------------------------------------------C--------
	// Rendering
	//-------------------------------------------------------------------------------------------------

	// Clear the screen - should always do this every frame. even if planning to render every pixel
	// because some GPUs gain efficiency by flagging the screen area as cleared @todo different colours / depth value
	void EngineDX::ClearBackBuffer()
	{
		float black[4] = {0, 0, 0, 0};
		mContext->ClearRenderTargetView(mBackBufferRTV, &black[0]);
		mContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}


	// Select a blending mode for subsequent rendering
	void EngineDX::SetRenderMode(RenderMode renderMode)
	{
		mCircleVS->Set(mContext);
		mCircleGS->Set(mContext);
		mCirclePS->Set(mContext);

		if (renderMode == RenderMode::Cutout)
		{
			mContext->OMSetDepthStencilState(mDepthEnable, 0);
			mContext->OMSetBlendState(mBlendNone, nullptr, 0xffffffff);
		}
		else if (renderMode == RenderMode::Additive)
		{
			mContext->OMSetDepthStencilState(mDepthWritesDisable, 0);
			mContext->OMSetBlendState(mBlendAdditive, nullptr, 0xffffffff);
		}
		else if (renderMode == RenderMode::Multiplicative)
		{
			mContext->OMSetDepthStencilState(mDepthWritesDisable, 0);
			mContext->OMSetBlendState(mBlendAdditive, nullptr, 0xffffffff);
		}
		else if (renderMode == RenderMode::AlphaBlending)
		{
			mContext->OMSetDepthStencilState(mDepthWritesDisable, 0);
			mContext->OMSetBlendState(mBlendAlpha, nullptr, 0xffffffff);
		}
	}


	// Display back buffer, optionally request vertical synchronisation
	void EngineDX::Present(bool vSync /*= false*/)
	{
		mSwapChain->Present((vSync ? 1 : 0), 0);
	}

	void EngineDX::Render(const CircleGPU* data, uint32_t count)
	{
		// Map vertex buffer and copy circle data in
		D3D11_MAPPED_SUBRESOURCE bufferData;
		if (FAILED(mContext->Map(mCirclesVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferData)))
		{
			return;
		}
		memcpy(bufferData.pData, data, count * sizeof(CircleGPU));
		mContext->Unmap(mCirclesVertexBuffer, 0);

		// Update constant buffer with backbuffer size for GS NDC conversion
		D3D11_MAPPED_SUBRESOURCE cbData;
		if (SUCCEEDED(mContext->Map(mPerFrameCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData)))
		{
			mPerFrameCBStruct.backbufferSize[0] = static_cast<float>(mBackBufferSize.x());
			mPerFrameCBStruct.backbufferSize[1] = static_cast<float>(mBackBufferSize.y());

#ifdef ENABLE_3D
			mPerFrameCBStruct.is3D = 1.0f;
#else
			mPerFrameCBStruct.is3D = 0.0f;
#endif

			memcpy(cbData.pData, &mPerFrameCBStruct, sizeof(mPerFrameCBStruct));
			mContext->Unmap(mPerFrameCB, 0);
		}

		// Set shaders
		mCircleVS->Set(mContext);
		mCircleGS->Set(mContext);
		mCirclePS->Set(mContext);

		// Bind vertex buffer and draw
		UINT vertexSize = sizeof(CircleGPU);
		UINT offset = 0;
		mContext->IASetVertexBuffers(0, 1, &mCirclesVertexBuffer.p, &vertexSize, &offset);
		mContext->Draw(count, 0);
	}


	//-------------------------------------------------------------------------------------------------
	// Debugging
	//-------------------------------------------------------------------------------------------------
#ifdef _DEBUG
// Test for leaks when object is destroyed
EngineDX::LeakTest::~LeakTest()
{
  // DXGI/DX11 memory leak information on object destruction
  typedef HRESULT(__stdcall *fPtr)(const IID&, void**); 
  HMODULE hDll = LoadLibrary("dxgidebug.dll");
  if (hDll)
  {
    CComPtr<IDXGIDebug> DXGIDebug;
    fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress( hDll, "DXGIGetDebugInterface" ); 
    if (DXGIGetDebugInterface) DXGIGetDebugInterface( __uuidof(DXGIDebug), (void**)(&DXGIDebug) );
    DXGIDebug->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL );
  }
}

// Initialise Direct3D debugging messages and breakpoints, call after device creation
void EngineDX::InitDebugging()
{
  CComPtr<ID3D11Debug> d3dDebug;
  if(SUCCEEDED(mDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
  {
    CComPtr<ID3D11InfoQueue> d3dInfoQueue;
    if( SUCCEEDED(mDevice->QueryInterface( __uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
    {
      d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
      d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR,      true);
    }
  }
}
#endif
} // namespaces
