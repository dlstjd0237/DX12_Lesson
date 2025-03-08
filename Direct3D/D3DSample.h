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

	// Tree �Է� ��ġ
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeInputLayout;


	// ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;
	BYTE* mObjectMappedData = nullptr;
	UINT mObjectByteSize = 0;

	// ���� ��� ����
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// ���� ��� ����
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// ���̴� ���ҽ�(�ؽ�ó) ������ ��
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	// ��ī�̹ڽ� �ؽ�ó �ε��� ����
	UINT mSkyboxTextureHeapIndex = -1;

	// Ʈ�� �ؽ�ó �ε��� ����
	UINT mTreeTextureHeapIndex = -1;

	//��Ʈ �ñ״�ó
	ComPtr<ID3D12RootSignature> mRootSignatrue = nullptr;

	// ������ ��
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	ComPtr<ID3D12PipelineState> mPSOs[(int)ERenderLayer::Count];
private:
	// ���ϵ��� ��
	std::unordered_map<std::string, std::unique_ptr<GeometryInfo>> mGeometries;

	// �ؽ��� ��
	std::unordered_map<std::string, std::unique_ptr<TexturInfo>> mTextures;

	// ���� ��
	std::unordered_map<std::string, std::unique_ptr<MaterialInfo>> mMaterials;

	// ������ �� ������Ʈ ����Ʈ
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;

	// PSO �� ������ ������Ʈ ����Ʈ
	std::vector<RenderItem*> mRenderItemLayer[(int)ERenderLayer::Count];

	// �ذ� �ִϸ��̼� ����
	BoneAnimation mSkullAnimation;

	float mAnimTimePos = 0.0f;

	RenderItem* mSkullRenderItem = nullptr;
	XMFLOAT4X4 mSkullWorld = MathHelper::Identity4x4();

private:
	// ���� / �þ� / ���� ���
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ī�޶� ��ġ
	XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };

	// ���� ��ǥ ���� ��
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0,0 };
};
