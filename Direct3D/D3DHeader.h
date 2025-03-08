#pragma once

#include "D3DApp.h"

#define MAX_LIGHT 16

enum class ERenderLayer : int {
	Opaque = 0,			// �Ϲ� ������Ʈ
	Transparent,		// ���� ���� ������Ʈ
	AlphaTested,		// �߶󳻱� ������Ʈ
	AlphaTestedTree,	// ���� ������Ʈ
	Skybox,				// ��ī�̹ڽ� ������Ʈ
	Count
};

enum class ETextureType : int
{
	TEXTURE2D = 0,
	TEXTURECUBE,
	TEXTURE2DARRAY,
	Count
};

// Tree Shader�� ����ü
struct TreeVertex {
	XMFLOAT3 Pos;
	XMFLOAT2 Size;
};

// Defualt Shader�� ����ü
struct Vertex {
	XMFLOAT3	Pos;
	XMFLOAT3	Normal;
	XMFLOAT2	Uv;
	XMFLOAT3	TangentU;
};

// ������Ʈ ���� ��� ����
struct ObjectConstants
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

// ���� ��� ����
struct MatConstants {
	XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f,0.01f,0.01f };
	float Roughness = 0.25f;
	UINT Texture_On = 0;
	UINT Normal_On = 0;
	XMFLOAT2 padding = { 0.0f,0.0f };
};

// ����Ʈ ����ü
struct LightInfo {
	UINT  LightType = 0; // 0: Direction, 1:Point, 2:Spot
	XMFLOAT3 Strength = { 0.5f,0.5f,0.5f };
	float FalloffStart = 1.0f;
	XMFLOAT3 Position = { 0.0f,0.0f,0.f };
	float FalloffEnd = 10.0f;
	XMFLOAT3 Direction = { 0.0f,-1.0f,0.0f };
	float SpotPower = 64.0f;
	XMFLOAT3 padding = { 0.0f,0.0f,0.0f };
};

// ���� ��� ����
struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();

	XMFLOAT4	AmbientLight = { 0.0f,0.0f,0.0f,1.0f };
	XMFLOAT3	EyePosW = { 0.0f,0.0f,0.0f };
	UINT		LightCount;
	LightInfo Lights[MAX_LIGHT];

	XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float FogStart = 5.0f;
	float FogRange = 150.0f;
	XMFLOAT2 padding = { 0.0f, 0.0f };
};

// ������Ʈ ����ü
struct GeometryInfo {

	std::string Name;

	// ���� ���� 
	D3D12_VERTEX_BUFFER_VIEW		VertexBufferView = {};
	ComPtr<ID3D12Resource>			VertexBuffer = nullptr;
	ComPtr<ID3D12Resource>			VertexBufferUploader = nullptr;

	// �ε��� ����
	D3D12_INDEX_BUFFER_VIEW			IndexBufferView = {};
	ComPtr<ID3D12Resource>			IndexBuffer = nullptr;
	ComPtr<ID3D12Resource>			IndexBufferUploader = nullptr;

	// ���� ����
	UINT VertexCount = 0;

	// �ε��� ����
	UINT IndexCount = 0;
};

// �ؽ�ó ����
struct TexturInfo {
	std::string Name;

	std::wstring Filename;

	int SrvHeapIndex = -1;

	ETextureType TextureType = ETextureType::TEXTURE2D;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

// ���� ����
struct MaterialInfo {
	std::string Name;

	UINT MatCBIndex = -1;

	TexturInfo* Tex = nullptr;
	int Texture_On = 0;

	TexturInfo* Normal = nullptr;
	int Normal_On = 0;

	XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f,0.01f,0.01f };
	float Roughness = 0.25f;
};

struct RenderItem {
	RenderItem() = default;

	UINT ObjCBIndex = -1;
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	D3D_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	GeometryInfo* Geo = nullptr;
	MaterialInfo* Mat = nullptr;
};