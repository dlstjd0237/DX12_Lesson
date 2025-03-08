#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <WindowsX.h>
#include <DirectXColors.h>

#include "d3dUtil.h"
#include "Camera.h"
#include "GameTimer.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "DDSTextureLoader.h"
#include "GeometryGenerator.h"
#include "AnimationHelper.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

class D3DApp
{
protected:
	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	static D3DApp* GetApp();

	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

	int Run();

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void CalculateFrameStats();

public:
	virtual void OnResize();
	virtual bool Initialize();

protected:
	virtual void Update(float deltaTime) = 0;
	virtual void LateUpdate(float deltaTime) = 0;
	virtual void DrawBegin() = 0;
	virtual void Draw() = 0;
	virtual void DrawEnd() = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
	bool InitMainWindow();

protected:
	bool InitDirect3D();				//Window가 생성된 후 Initialize에서 호출 
	void CreateDevice();
	void CreateCommandObjects();		//커멘드 관련 오브젝트 한번에 처리
	void CreateCommandFence();
	void Create4xMsaaState();			//멀티 샘플링
	void CreateSwapChain();				//더블 버퍼링
	void CreateDescriptorSize();		//서술자 사이즈 정의
	void CreateRtvDescriptorHeaps();	//BackBuffer의 DescriptorHeaps만들기
	void CreateDsvDescriptorHeaps();
	void CreateViewport();


public:
	void FlushCommandQueue();
	ID3D12Resource* GetCurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView()const;

protected:
	ComPtr<IDXGIFactory4>	mdxgiFactory; //GPU장치에 관련된 
	ComPtr<ID3D12Device>	md3dDevice;
	ComPtr<ID3D12CommandQueue>			mCommandQueue;
	ComPtr<ID3D12CommandAllocator>		mCommandAlloc;
	ComPtr<ID3D12GraphicsCommandList>	mCommandList;

	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	bool m4xMsaaState = false;
	UINT m4xMsaaQuality;

	ComPtr<IDXGISwapChain> mSwapChain;
	static const int SwapChainBufferCount = 2;
	int mCurrentBackBufferIndex = 0; //어떤애가 백버퍼의 역할을 할 것인가
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	ComPtr<ID3D12Resource>mSwapChainBuffer[SwapChainBufferCount]; //이 리소스는 GPU가 접근못함 그렇기에 View와 Descriptor가 필요함
	D3D12_CPU_DESCRIPTOR_HANDLE mSwapChainBufferView[SwapChainBufferCount] = {};

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthStencilBufferView = {};

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

protected:
	static D3DApp* mApp;

	HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND      mhMainWnd = nullptr; // main window handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	std::wstring mMainWndCaption = L"d3d App";

	int mClientWidth = 800;
	int mClientHeight = 600;

	// Used to keep track of the 밺elta-time?and game time (?.4).
	GameTimer mTimer;
};

