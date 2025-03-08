#pragma once

#include "D3DHeader.h"


class Direct3DApp : public D3DApp
{
public:
	Direct3DApp(HINSTANCE hInstance);
	~Direct3DApp();

public:
	virtual void OnResize()override;
	virtual bool Initialize()override;

private:
	virtual void Update(float deltaTime) override;
	virtual void LateUpdate(float deltaTime) override;
	virtual void DrawBegin() override;
	virtual void Draw() override;
	virtual void DrawEnd() override;

private:
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void UpdateCamera(float deltaTime);
	void UpdateSkullAnimation(float deltaTime);
	void UpdateObjectCB(float deltaTime);
	void UpdatePassCB(float deltaTime);

private:
	void BuildInputLayout();
	void BuildGeometry();
	void LoadTextures();
	void BuildMaterials();
	void BuildSkullAnimation();
	void BuildRenderItems();
	void BuildShaders();
	void BuildConstantBuffers();
	void BuildMaterialConstants();
	void BuildSrvDescriptorHeap();
	void BuildRootSignature();
	void BuildPSO();

private:
	void CreateBoxGeometry();
	void CreateGridGeometry();
	void CreateSphereGeometry();
	void CreateCylinderGeometry();
	void CreateSkullGeometry();
	void CreateTreeGeometry();

private:
	void DrawRenderItems(ERenderLayer DrawRenderLayer = ERenderLayer::Opaque);

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// Tree 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeInputLayout;


	// 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mObjectCB = nullptr;
	BYTE* mObjectMappedData = nullptr;
	UINT mObjectByteSize = 0;

	// 재질 상수 버퍼
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// 공용 상수 버퍼
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// 쉐이더 리소스(텍스처) 서술자 힙
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	// 스카이박스 텍스처 인덱스 변수
	UINT mSkyboxTextureHeapIndex = -1;

	// 트리 텍스처 인덱스 변수
	UINT mTreeTextureHeapIndex = -1;

	//루트 시그니처
	ComPtr<ID3D12RootSignature> mRootSignatrue = nullptr;

	// 쉐이터 맵
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	ComPtr<ID3D12PipelineState> mPSOs[(int)ERenderLayer::Count];
private:
	// 기하도형 맵
	std::unordered_map<std::string, std::unique_ptr<GeometryInfo>> mGeometries;

	// 텍스쳐 맵
	std::unordered_map<std::string, std::unique_ptr<TexturInfo>> mTextures;

	// 재질 맵
	std::unordered_map<std::string, std::unique_ptr<MaterialInfo>> mMaterials;

	// 랜더링 할 오브젝트 리스트
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;

	// PSO 별 렌더링 오브젝트 리스트
	std::vector<RenderItem*> mRenderItemLayer[(int)ERenderLayer::Count];

	// 해골 애니메이션 관련
	BoneAnimation mSkullAnimation;

	float mAnimTimePos = 0.0f;

	RenderItem* mSkullRenderItem = nullptr;
	XMFLOAT4X4 mSkullWorld = MathHelper::Identity4x4();

private:
	// 월드 / 시야 / 투영 행렬
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// 카메라 위치
	XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };

	// 구면 좌표 제어 값
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// 마우스 좌표
	POINT mLastMousePos = { 0,0 };
};
