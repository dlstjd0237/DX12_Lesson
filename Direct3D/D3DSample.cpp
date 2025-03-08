// InitDirect3D.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//


#include "D3DSample.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Direct3DApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

Direct3DApp::Direct3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

Direct3DApp::~Direct3DApp()
{
}

void Direct3DApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);

}

bool Direct3DApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	//명령 목록 재설정
	ThrowIfFailed(mCommandList->Reset(mCommandAlloc.Get(), nullptr));

	//초기화 명령 추가
	BuildInputLayout();
	BuildGeometry();
	LoadTextures();
	BuildMaterials();
	BuildSkullAnimation();
	BuildRenderItems();
	BuildShaders();
	BuildConstantBuffers();
	BuildMaterialConstants();
	BuildSrvDescriptorHeap();
	BuildRootSignature();
	BuildPSO();

	//초기화 명령 실행
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();


	return true;
}

void Direct3DApp::Update(float deltaTime)
{
	UpdateSkullAnimation(deltaTime);
	UpdateObjectCB(deltaTime);
}

void Direct3DApp::LateUpdate(float deltaTime)
{
	UpdateCamera(deltaTime);
	UpdatePassCB(deltaTime);
}

void Direct3DApp::DrawBegin()
{
	ThrowIfFailed(mCommandAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mCommandAlloc.Get(), nullptr));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(GetRenderTargetView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &GetRenderTargetView(), true, &GetDepthStencilView());

}

void Direct3DApp::Draw()
{
	mCommandList->SetGraphicsRootSignature(mRootSignatrue.Get());

	// 공용 상수 버퍼 렌더링 파이프라인 묶기
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	// SRV 텍스처 서술자 힙 렌더링 파이프라인 묶기
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 스카이박스 텍스처 힙을 렌더링 파이프라인에 묶기
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyboxTextureHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	// 트리 텍스처 힙을 렌더링 파이프라인에 묶기
	CD3DX12_GPU_DESCRIPTOR_HANDLE treeTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	treeTexDescriptor.Offset(mTreeTextureHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(6, treeTexDescriptor);

	// 랜더링 오브젝트 처리
	mCommandList->SetPipelineState(mPSOs[(int)ERenderLayer::Opaque].Get());
	DrawRenderItems(ERenderLayer::Opaque);

	mCommandList->SetPipelineState(mPSOs[(int)ERenderLayer::AlphaTested].Get());
	DrawRenderItems(ERenderLayer::AlphaTested);

	mCommandList->SetPipelineState(mPSOs[(int)ERenderLayer::AlphaTestedTree].Get());
	DrawRenderItems(ERenderLayer::AlphaTestedTree);

	mCommandList->SetPipelineState(mPSOs[(int)ERenderLayer::Transparent].Get());
	DrawRenderItems(ERenderLayer::Transparent);

	mCommandList->SetPipelineState(mPSOs[(int)ERenderLayer::Skybox].Get());
	DrawRenderItems(ERenderLayer::Skybox);



}

void Direct3DApp::DrawEnd()
{
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* comsList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(comsList), comsList);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void Direct3DApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}
void Direct3DApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}
void Direct3DApp::OnMouseMove(WPARAM btnState, int x, int y) {

	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 3.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;

}

void Direct3DApp::UpdateCamera(float deltaTime)
{
	// 구면 좌표 -> 직교 좌표
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);

	// 시야 행렬
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

}

void Direct3DApp::UpdateSkullAnimation(float deltaTime)
{
	mAnimTimePos += deltaTime;
	if (mAnimTimePos >= mSkullAnimation.GetEndTime()) {
		mAnimTimePos = 0.0f;
	}

	mSkullAnimation.Interpolate(mAnimTimePos, mSkullWorld);
	mSkullRenderItem->World = mSkullWorld;
}

void Direct3DApp::UpdateObjectCB(float deltaTime)
{
	for (auto& e : mRenderItems) {
		XMMATRIX world = XMLoadFloat4x4(&e->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

		UINT elementIndex = e->ObjCBIndex;
		UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
		memcpy(&mObjectMappedData[elementIndex * elementByteSize], &objConstants, sizeof(objConstants));
	}
}

void Direct3DApp::UpdatePassCB(float deltaTime)
{
	PassConstants mainPass;
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMStoreFloat4x4(&mainPass.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mainPass.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mainPass.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mainPass.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mainPass.ViewProj, XMMatrixTranspose(viewProj));

	mainPass.AmbientLight = { 0.25f,0.25f,0.35f,1.0f };
	mainPass.EyePosW = mEyePos;

	mainPass.LightCount = 3;
	mainPass.Lights[0].LightType = 0;
	mainPass.Lights[0].Direction = { 0.57735f,-0.57735f,0.57735f };
	mainPass.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };

	mainPass.Lights[1].LightType = 0;
	mainPass.Lights[1].Direction = { -0.57735f,-0.57735f,0.57735f };
	mainPass.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };

	mainPass.Lights[2].LightType = 0;
	mainPass.Lights[2].Direction = { 0.0f,-0.707f,-0.707f };
	mainPass.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	memcpy(&mPassMappedData[0], &mainPass, sizeof(mainPass));

}

void Direct3DApp::BuildInputLayout()
{
	mInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	mTreeInputLayout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"SIZE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}

void Direct3DApp::BuildGeometry()
{
	CreateBoxGeometry();
	CreateGridGeometry();
	CreateSphereGeometry();
	CreateCylinderGeometry();
	CreateSkullGeometry();
	CreateTreeGeometry();
}

void Direct3DApp::LoadTextures()
{
	UINT indexCount = 0;

	auto bricksTex = std::make_unique<TexturInfo>();
	bricksTex->Name = "BRICKS_TEX";
	bricksTex->Filename = L"Textures/bricks2.dds";
	bricksTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		bricksTex->Filename.c_str(),
		bricksTex->Resource,
		bricksTex->UploadHeap));
	mTextures[bricksTex->Name] = std::move(bricksTex);

	auto bricksNormalTex = std::make_unique<TexturInfo>();
	bricksNormalTex->Name = "BRICKS_NORMAL";
	bricksNormalTex->Filename = L"Textures/bricks2_nmap.dds";
	bricksNormalTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		bricksNormalTex->Filename.c_str(),
		bricksNormalTex->Resource,
		bricksNormalTex->UploadHeap));
	mTextures[bricksNormalTex->Name] = std::move(bricksNormalTex);

	auto StoneTex = std::make_unique<TexturInfo>();
	StoneTex->Name = "STONE_TEX";
	StoneTex->Filename = L"Textures/stone.dds";
	StoneTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		StoneTex->Filename.c_str(),
		StoneTex->Resource,
		StoneTex->UploadHeap));
	mTextures[StoneTex->Name] = std::move(StoneTex);

	auto TileTex = std::make_unique<TexturInfo>();
	TileTex->Name = "TILE_TEX";
	TileTex->Filename = L"Textures/tile.dds";
	TileTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		TileTex->Filename.c_str(),
		TileTex->Resource,
		TileTex->UploadHeap));
	mTextures[TileTex->Name] = std::move(TileTex);

	auto tileNormalTex = std::make_unique<TexturInfo>();
	tileNormalTex->Name = "TILE_NORMAL";
	tileNormalTex->Filename = L"Textures/tile_nmap.dds";
	tileNormalTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		tileNormalTex->Filename.c_str(),
		tileNormalTex->Resource,
		tileNormalTex->UploadHeap));
	mTextures[tileNormalTex->Name] = std::move(tileNormalTex);

	auto fenceTex = std::make_unique<TexturInfo>();
	fenceTex->Name = "FENCE_TEX";
	fenceTex->Filename = L"Textures/WireFence.dds";
	fenceTex->SrvHeapIndex = indexCount++;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		fenceTex->Filename.c_str(),
		fenceTex->Resource,
		fenceTex->UploadHeap));
	mTextures[fenceTex->Name] = std::move(fenceTex);

	auto skyboxTex = std::make_unique<TexturInfo>();
	skyboxTex->Name = "SKYBOX_TEX";
	skyboxTex->Filename = L"Textures/snowcube1024.dds";
	skyboxTex->SrvHeapIndex = indexCount++;
	skyboxTex->TextureType = ETextureType::TEXTURECUBE;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		skyboxTex->Filename.c_str(),
		skyboxTex->Resource,
		skyboxTex->UploadHeap));
	mSkyboxTextureHeapIndex = skyboxTex->SrvHeapIndex; // Skybox Heap Index
	mTextures[skyboxTex->Name] = std::move(skyboxTex);


	auto treeTex = std::make_unique<TexturInfo>();
	treeTex->Name = "TREE_TEX";
	treeTex->Filename = L"Textures/treeArray2.dds";
	treeTex->SrvHeapIndex = indexCount++;
	treeTex->TextureType = ETextureType::TEXTURE2DARRAY;
	ThrowIfFailed(CreateDDSTextureFromFile12(
		md3dDevice.Get(),
		mCommandList.Get(),
		treeTex->Filename.c_str(),
		treeTex->Resource,
		treeTex->UploadHeap));
	mTreeTextureHeapIndex = treeTex->SrvHeapIndex;
	mTextures[treeTex->Name] = std::move(treeTex);
}

void Direct3DApp::BuildMaterials()
{
	UINT indexCount = 0;
	auto bricks = std::make_unique<MaterialInfo>();
	bricks->Name = "BRICKS";
	bricks->MatCBIndex = indexCount++;
	bricks->Tex = mTextures["BRICKS_TEX"].get();
	bricks->Normal = mTextures["BRICKS_NORMAL"].get();
	bricks->Normal_On = 1;
	bricks->Texture_On = 1;
	bricks->DiffuseAlbedo = XMFLOAT4(Colors::Gray);
	bricks->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks->Roughness = 0.7f;
	mMaterials[bricks->Name] = std::move(bricks);

	auto stone = std::make_unique<MaterialInfo>();
	stone->Name = "STONE";
	stone->MatCBIndex = indexCount++;
	stone->Tex = mTextures["STONE_TEX"].get();
	stone->Texture_On = 1;
	stone->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	stone->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone->Roughness = 0.7f;
	mMaterials[stone->Name] = std::move(stone);

	auto tile = std::make_unique<MaterialInfo>();
	tile->Name = "TILE";
	tile->MatCBIndex = indexCount++;
	tile->Tex = mTextures["TILE_TEX"].get();
	tile->Normal = mTextures["TILE_NORMAL"].get();
	tile->Texture_On = 1;
	tile->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
	tile->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile->Roughness = 0.3f;
	mMaterials[tile->Name] = std::move(tile);

	auto Skull = std::make_unique<MaterialInfo>();
	Skull->Name = "SKULL";
	Skull->MatCBIndex = indexCount++;
	Skull->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	Skull->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	Skull->Roughness = 0.3f;
	mMaterials[Skull->Name] = std::move(Skull);

	auto fence = std::make_unique<MaterialInfo>();
	fence->Name = "FENCE";
	fence->MatCBIndex = indexCount++;
	fence->Tex = mTextures["FENCE_TEX"].get();
	fence->Texture_On = 1;
	fence->DiffuseAlbedo = XMFLOAT4(Colors::White);
	fence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	fence->Roughness = 0.25f;
	mMaterials[fence->Name] = std::move(fence);

	auto skybox = std::make_unique<MaterialInfo>();
	skybox->Name = "SKYBOX";
	skybox->MatCBIndex = indexCount++;
	skybox->Tex = mTextures["SKYBOX_TEX"].get();
	skybox->Texture_On = 1;
	skybox->DiffuseAlbedo = XMFLOAT4(Colors::White);
	skybox->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	skybox->Roughness = 1.0f;
	mMaterials[skybox->Name] = std::move(skybox);

	auto mirror = std::make_unique<MaterialInfo>();
	mirror->Name = "MIRROR";
	mirror->MatCBIndex = indexCount++;
	mirror->Texture_On = 1; //이거 사실 없어도 문제 없음
	mirror->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mirror->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirror->Roughness = 0.1f;
	mMaterials[mirror->Name] = std::move(mirror);

	auto tree = std::make_unique<MaterialInfo>();
	tree->Name = "TREE";
	tree->MatCBIndex = indexCount++;
	tree->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tree->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	tree->Roughness = 1.0f;
	mMaterials[tree->Name] = std::move(tree);
}

void Direct3DApp::BuildSkullAnimation()
{
	XMVECTOR q0 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(30.0f));		//30 도
	XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(1.0f, 1.0f, 2.0f, 0.0f), XMConvertToRadians(45.0f));		//45 도
	XMVECTOR q2 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(-30.0f));	//-30 도
	XMVECTOR q3 = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMConvertToRadians(70.0f));	//-30 도

	mSkullAnimation.Keyframes.resize(5);

	mSkullAnimation.Keyframes[0].TimePos = 0.0f;
	mSkullAnimation.Keyframes[0].Translation = XMFLOAT3(-7.0f, 0.0f, 0.0f);
	mSkullAnimation.Keyframes[0].Scale = XMFLOAT3(-0.25f, 0.25f, 0.25f);
	XMStoreFloat4(&mSkullAnimation.Keyframes[0].RotationQuat, q0);

	mSkullAnimation.Keyframes[1].TimePos = 2.0f;
	mSkullAnimation.Keyframes[1].Translation = XMFLOAT3(0.0f, 2.0f, 10.0f);
	mSkullAnimation.Keyframes[1].Scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
	XMStoreFloat4(&mSkullAnimation.Keyframes[1].RotationQuat, q1);

	mSkullAnimation.Keyframes[2].TimePos = 4.0f;
	mSkullAnimation.Keyframes[2].Translation = XMFLOAT3(7.0f, 0.0f, 0.0f);
	mSkullAnimation.Keyframes[2].Scale = XMFLOAT3(0.25f, 0.25f, 0.25f);
	XMStoreFloat4(&mSkullAnimation.Keyframes[2].RotationQuat, q2);

	mSkullAnimation.Keyframes[3].TimePos = 6.0f;
	mSkullAnimation.Keyframes[3].Translation = XMFLOAT3(0.0f, 1.0f, -10.0f);
	mSkullAnimation.Keyframes[3].Scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
	XMStoreFloat4(&mSkullAnimation.Keyframes[3].RotationQuat, q3);


	mSkullAnimation.Keyframes[4].TimePos = 8.0f;
	mSkullAnimation.Keyframes[4].Translation = XMFLOAT3(-7.0f, 0.0f, 0.0f);
	mSkullAnimation.Keyframes[4].Scale = XMFLOAT3(0.25f, 0.25f, 0.25f);
	XMStoreFloat4(&mSkullAnimation.Keyframes[4].RotationQuat, q0);
}

void Direct3DApp::BuildRenderItems()
{
	UINT indexCount = 0;

	auto skyboxItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyboxItem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	skyboxItem->ObjCBIndex = indexCount++;
	skyboxItem->Geo = mGeometries["BOX"].get();
	skyboxItem->Mat = mMaterials["SKYBOX"].get();
	mRenderItemLayer[(int)ERenderLayer::Skybox].push_back(skyboxItem.get());
	mRenderItems.push_back(std::move(skyboxItem));

	auto boxItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxItem->ObjCBIndex = indexCount++;
	boxItem->Geo = mGeometries["BOX"].get();
	boxItem->Mat = mMaterials["FENCE"].get();
	mRenderItemLayer[(int)ERenderLayer::AlphaTested].push_back(boxItem.get());
	mRenderItems.push_back(std::move(boxItem));

	auto gridItem = std::make_unique<RenderItem>();
	gridItem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridItem->World, XMMatrixScaling(5.0f, 5.0f, 5.0f));
	XMStoreFloat4x4(&gridItem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridItem->ObjCBIndex = indexCount++;
	gridItem->Geo = mGeometries["GRID"].get();
	gridItem->Mat = mMaterials["TILE"].get();
	mRenderItemLayer[(int)ERenderLayer::Opaque].push_back(gridItem.get());
	mRenderItems.push_back(std::move(gridItem));

	auto skullItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skullItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullItem->ObjCBIndex = indexCount++;
	skullItem->Geo = mGeometries["SKULL"].get();
	skullItem->Mat = mMaterials["SKULL"].get();
	mRenderItemLayer[(int)ERenderLayer::Transparent].push_back(skullItem.get());
	mSkullRenderItem = skullItem.get();
	mRenderItems.push_back(std::move(skullItem));

	for (UINT i = 0; i < 5; ++i)
	{
		auto leftCylItem = std::make_unique<RenderItem>();
		auto rightCylItem = std::make_unique<RenderItem>();
		auto leftSphereItem = std::make_unique<RenderItem>();
		auto rightSphereItem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylItem->World, leftCylWorld);
		leftCylItem->ObjCBIndex = indexCount++;
		leftCylItem->Geo = mGeometries["CYLINDER"].get();
		leftCylItem->Mat = mMaterials["BRICKS"].get();
		mRenderItemLayer[(int)ERenderLayer::Opaque].push_back(leftCylItem.get());
		mRenderItems.push_back(std::move(leftCylItem));

		XMStoreFloat4x4(&rightCylItem->World, rightCylWorld);
		rightCylItem->ObjCBIndex = indexCount++;
		rightCylItem->Geo = mGeometries["CYLINDER"].get();
		rightCylItem->Mat = mMaterials["BRICKS"].get();
		mRenderItemLayer[(int)ERenderLayer::Opaque].push_back(rightCylItem.get());
		mRenderItems.push_back(std::move(rightCylItem));

		XMStoreFloat4x4(&leftSphereItem->World, leftSphereWorld);
		leftSphereItem->ObjCBIndex = indexCount++;
		leftSphereItem->Geo = mGeometries["SPHERE"].get();
		leftSphereItem->Mat = mMaterials["MIRROR"].get();
		mRenderItemLayer[(int)ERenderLayer::Opaque].push_back(leftSphereItem.get());
		mRenderItems.push_back(std::move(leftSphereItem));

		XMStoreFloat4x4(&rightSphereItem->World, rightSphereWorld);
		rightSphereItem->ObjCBIndex = indexCount++;
		rightSphereItem->Geo = mGeometries["SPHERE"].get();
		rightSphereItem->Mat = mMaterials["MIRROR"].get();
		mRenderItemLayer[(int)ERenderLayer::Opaque].push_back(rightSphereItem.get());
		mRenderItems.push_back(std::move(rightSphereItem));

		//Tree
		auto treeItem = std::make_unique<RenderItem>();
		treeItem->World = MathHelper::Identity4x4();
		treeItem->ObjCBIndex = indexCount++;
		treeItem->Geo = mGeometries["TREE"].get();
		treeItem->Mat = mMaterials["TREE"].get();
		treeItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		mRenderItemLayer[(int)ERenderLayer::AlphaTestedTree].push_back(treeItem.get());
		mRenderItems.push_back(std::move(treeItem));
	}
}

void Direct3DApp::BuildShaders()
{
	const D3D_SHADER_MACRO defines[] = {
		"FOG","1",
		NULL,NULL
	};

	const D3D_SHADER_MACRO alphaTestedDefines[] = {
		"FOG","1",
		"ALPHA_TEST","1",
		NULL,NULL
	};

	mShaders["VS"] = d3dUtil::CompileShader(L"Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["PS"] = d3dUtil::CompileShader(L"Default.hlsl", defines, "PS", "ps_5_0");
	mShaders["AlphaTestedPS"] = d3dUtil::CompileShader(L"Default.hlsl", alphaTestedDefines, "PS", "ps_5_0");


	mShaders["SkyboxVS"] = d3dUtil::CompileShader(L"Skybox.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["SkyboxPS"] = d3dUtil::CompileShader(L"Skybox.hlsl", nullptr, "PS", "ps_5_0");

	mShaders["TreeVS"] = d3dUtil::CompileShader(L"Tree.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["TreeGS"] = d3dUtil::CompileShader(L"Tree.hlsl", nullptr, "GS", "gs_5_0");
	mShaders["TreePS"] = d3dUtil::CompileShader(L"Tree.hlsl", alphaTestedDefines, "PS", "ps_5_0");
}

void Direct3DApp::BuildConstantBuffers()
{
	// 오브젝트 상수 버퍼(월드정보)
	UINT size = sizeof(ObjectConstants);
	mObjectByteSize = ((size + 255) & ~255) * (UINT)mRenderItems.size();

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mObjectByteSize);
	D3D12_HEAP_PROPERTIES heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	md3dDevice->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mObjectCB));

	mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mObjectMappedData));

	// 재질 상수 버퍼
	size = sizeof(MatConstants);
	mMaterialByteSize = ((size + 255) & ~255) * (UINT)mMaterials.size();

	desc = CD3DX12_RESOURCE_DESC::Buffer(mMaterialByteSize);
	heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	md3dDevice->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mMaterialCB));

	mMaterialCB->Map(0, nullptr, reinterpret_cast<void**>(&mMaterialMappedData));

	// 메인 패스 상수 버퍼(ViewProj 정보)
	size = sizeof(PassConstants);
	mPassByteSize = (size + 255) & ~255;

	desc = CD3DX12_RESOURCE_DESC::Buffer(mPassByteSize);
	heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	md3dDevice->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mPassCB));

	mPassCB->Map(0, nullptr, reinterpret_cast<void**>(&mPassMappedData));
}

void Direct3DApp::BuildMaterialConstants()
{
	for (auto& e : mMaterials) {
		MaterialInfo* mat = e.second.get();

		MatConstants matConstants;
		matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
		matConstants.FresnelR0 = mat->FresnelR0;
		matConstants.Roughness = mat->Roughness;
		matConstants.Texture_On = mat->Tex ? 1 : 0;
		matConstants.Normal_On = mat->Normal ? 1 : 0;

		UINT elementIndex = mat->MatCBIndex;
		UINT elementByteSize = (sizeof(MatConstants) + 255) & ~255;
		memcpy(&mMaterialMappedData[elementIndex * elementByteSize], &matConstants, sizeof(MatConstants));
	}
}

void Direct3DApp::BuildSrvDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = (UINT)mTextures.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	for (auto& e : mTextures) {
		TexturInfo* tex = e.second.get();

		CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		hDescriptor.Offset(tex->SrvHeapIndex, mCbvSrvUavDescriptorSize);

		auto texResource = tex->Resource;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texResource->GetDesc().Format;

		switch (tex->TextureType)
		{
		case ETextureType::TEXTURE2D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			break;
		case ETextureType::TEXTURECUBE:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = texResource->GetDesc().MipLevels;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			break;
		case ETextureType::TEXTURE2DARRAY:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = -1.0f;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = texResource->GetDesc().DepthOrArraySize;
			break;
		}

		md3dDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, hDescriptor);
	}
}

void Direct3DApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE skyboxtexTable[] = {
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0), // t0 //스카이박스 렌더링 텍스처
	};

	CD3DX12_DESCRIPTOR_RANGE texTable[] = {
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,1), // t1 //오브젝트의 렌더링 텍스처
	};

	CD3DX12_DESCRIPTOR_RANGE normalTable[] = {
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,2), // t2 //오브젝트의 렌더링 노말맵
	};

	CD3DX12_DESCRIPTOR_RANGE treeTable[] = {
	CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,3),	   // t3 // 츠리의 텍스처 배열
	};

	CD3DX12_ROOT_PARAMETER params[7];
	params[0].InitAsConstantBufferView(0); // 0번 -> b0 : CBV ObejctCB
	params[1].InitAsConstantBufferView(1); // 1번 -> b1 : CBV MaterialCB
	params[2].InitAsConstantBufferView(2); // 2번 -> b2 : CBV PassCB
	params[3].InitAsDescriptorTable(_countof(skyboxtexTable), skyboxtexTable);  // 3번 -> t0 : SRV Skybox
	params[4].InitAsDescriptorTable(_countof(texTable), texTable);				// 4번 -> t1 : SRV Object Texture
	params[5].InitAsDescriptorTable(_countof(normalTable), normalTable);		// 5번 -> t2 : SRV Object Normal Texture
	params[6].InitAsDescriptorTable(_countof(treeTable), treeTable);			// 6번 -> t3 : SRV Tree Texture Array

	D3D12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0); // s0

	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(params), params, 1, &samplerDesc);
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;
	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature,
		&blobError);

	md3dDevice->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignatrue));
}

void Direct3DApp::BuildPSO()
{
	for (int i = 0; i < (int)ERenderLayer::Count; ++i)
		mPSOs[i] = nullptr;

	// ====Opaque Rendering Pipeline State=====
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignatrue.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["VS"]->GetBufferPointer()),
		mShaders["VS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["PS"]->GetBufferPointer()),
		mShaders["PS"]->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[(int)ERenderLayer::Opaque])));
	// ========================================


	// ====Transparent Rendering Pipeline State=====
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPSODesc = psoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparentBlendDesc;
	transparentBlendDesc.BlendEnable = true;
	transparentBlendDesc.LogicOpEnable = false;
	transparentBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparentBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparentBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparentBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparentBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparentBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparentBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparentBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPSODesc.BlendState.RenderTarget[0] = transparentBlendDesc;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPSODesc, IID_PPV_ARGS(&mPSOs[(int)ERenderLayer::Transparent])));
	// =============================================


	// ====AlphaTested Rendering Pipeline State====
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPSODes = psoDesc;
	alphaTestedPSODes.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["AlphaTestedPS"]->GetBufferPointer()),
		mShaders["AlphaTestedPS"]->GetBufferSize()
	};

	alphaTestedPSODes.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPSODes, IID_PPV_ARGS(&mPSOs[(int)ERenderLayer::AlphaTested])));
	// ============================================

	// ====Skybox Rendering Pipeline State====
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxPSODesc = psoDesc;
	skyboxPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyboxVS"]->GetBufferPointer()),
		mShaders["SkyboxVS"]->GetBufferSize()
	};
	skyboxPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyboxPS"]->GetBufferPointer()),
		mShaders["SkyboxPS"]->GetBufferSize()
	};
	skyboxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	skyboxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyboxPSODesc,
		IID_PPV_ARGS(&mPSOs[(int)ERenderLayer::Skybox])));
	// =======================================


	// ====Tree Rendering Pipeline State====
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treePSODesc = psoDesc;

	treePSODesc.InputLayout = { mTreeInputLayout.data(), (UINT)mTreeInputLayout.size() };
	treePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["TreeVS"]->GetBufferPointer()),
		mShaders["TreeVS"]->GetBufferSize()
	};
	treePSODesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["TreeGS"]->GetBufferPointer()),
		mShaders["TreeGS"]->GetBufferSize()
	};
	treePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["TreePS"]->GetBufferPointer()),
		mShaders["TreePS"]->GetBufferSize()
	};
	treePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treePSODesc,
		IID_PPV_ARGS(&mPSOs[(int)ERenderLayer::AlphaTestedTree])));
	// =====================================
}

void Direct3DApp::CreateBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData mesh = geoGen.CreateBox(1.5, 0.5f, 1.5f, 3);

	std::vector<Vertex> vertices(mesh.Vertices.size());

	for (size_t i = 0; i < mesh.Vertices.size(); ++i)
	{
		vertices[i].Pos = mesh.Vertices[i].Position;
		vertices[i].Normal = mesh.Vertices[i].Normal;
		vertices[i].Uv = mesh.Vertices[i].TexC;
		vertices[i].TangentU = mesh.Vertices[i].TangentU;
	}

	std::vector<std::int32_t> indices;
	indices.insert(indices.end(), std::begin(mesh.Indices32), std::end(mesh.Indices32));


	// 정점 버퍼 생성
	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "BOX";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void Direct3DApp::CreateGridGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData mesh = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);

	std::vector<Vertex> vertices(mesh.Vertices.size());

	for (size_t i = 0; i < mesh.Vertices.size(); ++i)
	{
		vertices[i].Pos = mesh.Vertices[i].Position;
		vertices[i].Normal = mesh.Vertices[i].Normal;
		vertices[i].Uv = mesh.Vertices[i].TexC;
		vertices[i].TangentU = mesh.Vertices[i].TangentU;
	}

	std::vector<std::int32_t> indices;
	indices.insert(indices.end(), std::begin(mesh.Indices32), std::end(mesh.Indices32));


	// 정점 버퍼 생성
	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "GRID";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void Direct3DApp::CreateSphereGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData mesh = geoGen.CreateSphere(0.5f, 20, 20);

	std::vector<Vertex> vertices(mesh.Vertices.size());

	for (size_t i = 0; i < mesh.Vertices.size(); ++i)
	{
		vertices[i].Pos = mesh.Vertices[i].Position;
		vertices[i].Normal = mesh.Vertices[i].Normal;
		vertices[i].Uv = mesh.Vertices[i].TexC;
		vertices[i].TangentU = mesh.Vertices[i].TangentU;
	}

	std::vector<std::int32_t> indices;
	indices.insert(indices.end(), std::begin(mesh.Indices32), std::end(mesh.Indices32));


	// 정점 버퍼 생성
	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "SPHERE";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void Direct3DApp::CreateCylinderGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData mesh = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	std::vector<Vertex> vertices(mesh.Vertices.size());

	for (size_t i = 0; i < mesh.Vertices.size(); ++i)
	{
		vertices[i].Pos = mesh.Vertices[i].Position;
		vertices[i].Normal = mesh.Vertices[i].Normal;
		vertices[i].Uv = mesh.Vertices[i].TexC;
		vertices[i].TangentU = mesh.Vertices[i].TangentU;
	}

	std::vector<std::int32_t> indices;
	indices.insert(indices.end(), std::begin(mesh.Indices32), std::end(mesh.Indices32));


	// 정점 버퍼 생성
	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "CYLINDER";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void Direct3DApp::CreateSkullGeometry()
{
	std::ifstream fin("Models/skull.txt");
	if (!fin) {
		MessageBox(0, L"Models.skull.txt not found", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}
	fin >> ignore >> ignore >> ignore;
	std::vector<std::int32_t>indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();


	// 정점 버퍼 생성
	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "SKULL";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);

}

void Direct3DApp::CreateTreeGeometry()
{
	static const int treeCount = 16;
	float randomRange = 45.0f;
	float treeSize = 20.0f;
	std::vector<TreeVertex> vertices(treeCount);
	for (UINT i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-randomRange, randomRange);
		float z = MathHelper::RandF(-randomRange, randomRange);
		float y = treeSize / 2.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(treeSize, treeSize);
	}

	std::vector<std::int32_t> indices(treeCount);
	for (UINT i = 0; i < treeCount; ++i)
	{
		indices[i] = i;
	}


	auto geo = std::make_unique<GeometryInfo>();
	geo->Name = "TREE";


	// 정점 버퍼 생성
	geo->VertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->VertexCount * sizeof(TreeVertex);

	geo->VertexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
	geo->VertexBufferView.StrideInBytes = sizeof(TreeVertex);
	geo->VertexBufferView.SizeInBytes = vbByteSize;

	// 인덱스 버퍼 생성
	geo->IndexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);

	geo->IndexBuffer = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
	geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void Direct3DApp::DrawRenderItems(ERenderLayer DrawRenderLayer)
{
	UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
	UINT matCBByteSize = (sizeof(MatConstants) + 255) & ~255;

	for (auto ri : mRenderItemLayer[(int)DrawRenderLayer])
	{
		// 개별 오브젝트 상수 버퍼 (월드 값)
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		// 재질 상수 버퍼
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = mMaterialCB->GetGPUVirtualAddress();
		matCBAddress += ri->Mat->MatCBIndex * matCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, matCBAddress);

		// 텍스처 쉐이더 리소스 뷰
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		if (ri->Mat->Tex) {
			tex.Offset(ri->Mat->Tex->SrvHeapIndex, mCbvSrvUavDescriptorSize);
			mCommandList->SetGraphicsRootDescriptorTable(4, tex);
		}


		// 노말맵 쉐이더 리소스 뷰
		CD3DX12_GPU_DESCRIPTOR_HANDLE normal(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		if (ri->Mat->Normal) {
			normal.Offset(ri->Mat->Normal->SrvHeapIndex, mCbvSrvUavDescriptorSize);
			mCommandList->SetGraphicsRootDescriptorTable(5, normal);
		}


		mCommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView);
		mCommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		mCommandList->DrawIndexedInstanced(ri->Geo->IndexCount, 1, 0, 0, 0);
	}
}
