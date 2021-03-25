#pragma once

#include "Renderer.h"
#include "DxgiInfoManager.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Tiny_obj_loader.h"

#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#pragma region structs
struct Vertex
{
	struct
	{
		float x;
		float y;
		float z;
	} pos;

	struct
	{
		float u;
		float v;
	} texCoord;

	bool operator==(const Vertex& other) const {
		return pos.x == other.pos.x && pos.y == other.pos.y && pos.z == other.pos.z;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<float>()(vertex.pos.x) ^ (hash<float>()(vertex.pos.y) << 1)) >> 1) ^ (hash<float>()(vertex.pos.z) << 1);
		}
	};
}

#pragma endregion structs

class Graphics {
public:
	Graphics(Renderer& renderer, std::string model_path, std::string texture_path);
	~Graphics(); //destructor
	void draw(Renderer* renderer, float angle, float x, float z);
	void loadModel(Renderer& renderer, std::string model_path);
	void loadTexture(std::string texture_path);
	void createMesh(Renderer& renderer);
	void createShaders(Renderer& renderer);
	void createRenderStates(Renderer& renderer);
	void negativeToPositive(int* a);
	void updateFarestPoint(int x, int y, int z);

private:
	Renderer* m_rendererPtr = nullptr;

	std::vector<Vertex> m_vertices;
	std::vector<unsigned short> m_indices;
	int m_farestPoint = 1;

	bool firstDraw = true;
	HRESULT buffer;
	int m_size = 0;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	D3D11_BUFFER_DESC* m_cbd = nullptr;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11BlendState* m_blendState = nullptr;

	ID3D11Resource* m_texture = nullptr;
	ID3D11ShaderResourceView* m_textureView = nullptr;
	ID3D11SamplerState* m_textSamplerState = nullptr;;
#ifndef NDEBUG
	DxgiInfoManager infoManager;
#endif
};