#include "Graphics.h"


#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "WICTextureLoader.h"


using namespace std;

#pragma region d3d11debug
HRESULT hr;

// graphics exception checking/throwing macros (some with dxgi infos)
#define GFX_EXCEPT_NOINFO(hr) Renderer::HrException( __LINE__,__FILE__,(hr) )
#define GFX_THROW_NOINFO(hrcall) if( FAILED( hr = (hrcall) ) ) throw Renderer::HrException( __LINE__,__FILE__,hr )

#ifndef NDEBUG
#define GFX_EXCEPT(hr) Renderer::HrException( __LINE__,__FILE__,(hr),infoManager.GetMessages() )
#define GFX_THROW_INFO(hrcall) infoManager.Set(); if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr)
#define GFX_DEVICE_REMOVED_EXCEPT(hr) Renderer::DeviceRemovedException( __LINE__,__FILE__,(hr),infoManager.GetMessages() )
#else
#define GFX_EXCEPT(hr) Renderer::HrException( __LINE__,__FILE__,(hr) )
#define GFX_THROW_INFO(hrcall) GFX_THROW_NOINFO(hrcall)
#define GFX_DEVICE_REMOVED_EXCEPT(hr) Renderer::DeviceRemovedException( __LINE__,__FILE__,(hr) )
#endif
#pragma endregion d3d11debug


//constructor
Graphics::Graphics(Renderer& renderer, string model_path, string texture_path) {
	m_rendererPtr = &renderer;
	loadModel(renderer, model_path);
	createMesh(renderer);
	createShaders(renderer);
	createRenderStates(renderer);
	loadTexture(texture_path);
}

//destructor
Graphics::~Graphics() {
	m_vertexBuffer->Release();
	m_indexBuffer->Release();
	m_constantBuffer->Release();
	m_vertexShader->Release();
	m_pixelShader->Release();
	m_inputLayout->Release();
	m_rasterizerState->Release();
	m_depthState->Release();
	m_blendState->Release();
}

void Graphics::draw(Renderer* renderer, float angle, float x, float z) {
	using namespace DirectX;
	auto deviceContext = renderer->getDeviceContext();

	// Bind the vertex buffer to pipeline
	UINT stride = sizeof(Vertex);
	UINT offset = 0u;
	deviceContext->IASetVertexBuffers(0u, 1u, &m_vertexBuffer, &stride, &offset);

	// Bind index buffer
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0u);

	float scalemultiplier = 1 / (float)m_farestPoint;
	XMVECTORF32 const vScale = { .4f * scalemultiplier, 0.55f * scalemultiplier, .4f * scalemultiplier};

	XMMATRIX cb =
	{
		XMMatrixTranspose(
			XMMatrixRotationZ(angle) *
			XMMatrixRotationY(0) *
			XMMatrixRotationX(-3.14 / 3) *
			XMMatrixTranslation(x, 0.0f, z + 4.0f)
		)
	};

	auto cbFinal = XMMatrixMultiply(XMMatrixScalingFromVector(vScale), cb);

	
 	D3D11_BUFFER_DESC cbd;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.MiscFlags = 0u;
	cbd.ByteWidth = sizeof(cbFinal);
	cbd.StructureByteStride = 0u;
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cbFinal;

	if(firstDraw) {
		buffer = renderer->getDevice()->CreateBuffer(&cbd, &csd, &m_constantBuffer);
		firstDraw = false;
	}

	D3D11_MAPPED_SUBRESOURCE resource;
	deviceContext->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, csd.pSysMem, cbd.ByteWidth);
	deviceContext->Unmap(m_constantBuffer, 0);

	GFX_THROW_INFO(buffer);

	// bind constant buffer to vertex shader
	deviceContext->VSSetConstantBuffers(0u, 1u, &m_constantBuffer);

	// bind texture to pixel shader
	deviceContext->PSSetShaderResources(0u, 1u, &m_textureView);
	deviceContext->PSSetSamplers(0u, 1u, &m_textSamplerState);

	// set render states
	deviceContext->RSSetState(m_rasterizerState);
	deviceContext->OMSetBlendState(m_blendState, NULL, 0xffffff);
	deviceContext->OMSetDepthStencilState(m_depthState, 1u);

	// bind vertex layout
	deviceContext->IASetInputLayout(m_inputLayout);

	// bind the triangle shaders
	deviceContext->VSSetShader(m_vertexShader, nullptr, 0u);
	deviceContext->PSSetShader(m_pixelShader, nullptr, 0u);

	// set primitive topology to triangle list (groups of 3 vertices)
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// draw
	deviceContext->DrawIndexed((UINT) m_size, 0u, 0u);
}

void Graphics::loadModel(Renderer& renderer, string model_path)
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str()))
		throw runtime_error("load model error " + warn + err);

	unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1 - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0) {
				updateFarestPoint(vertex.pos.x, vertex.pos.y, vertex.pos.z);
				uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}
				m_indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Graphics::updateFarestPoint(int x, int y, int z) {
	if (x < 0) negativeToPositive(&x);
	if (y < 0) negativeToPositive(&y);
	if (z < 0) negativeToPositive(&z);

	if (m_farestPoint < x) m_farestPoint = x;
	if (m_farestPoint < y) m_farestPoint = y;
	if (m_farestPoint < z) m_farestPoint = z;
}

void Graphics::negativeToPositive(int* a) {
	int b = *a * 2;
	*a -= b;
}

void Graphics::loadTexture(string texture_path) {
	wstring text;
	for (int i = 0; i < texture_path.length(); ++i)
		text += wchar_t(texture_path[i]);
	const wchar_t* file = text.c_str();

	GFX_THROW_INFO(DirectX::CreateWICTextureFromFile(m_rendererPtr->getDevice(), file, &m_texture, &m_textureView));

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	m_rendererPtr->getDevice()->CreateSamplerState(&sampDesc, &m_textSamplerState);
}
	

void Graphics::createMesh(Renderer& renderer) {
	
	// create vertex buffer
	D3D11_BUFFER_DESC bd = {}; //vertexBufferDesc
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(Vertex) * m_vertices.size();
	bd.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = m_vertices.data();
	GFX_THROW_INFO(renderer.getDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer));


	m_size = m_indices.size();
	// create index buffer
	D3D11_BUFFER_DESC ibd = {}; //indicesBufferDesc
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = sizeof(unsigned short) * m_indices.size();
	ibd.StructureByteStride = sizeof(unsigned short);

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = m_indices.data();
	GFX_THROW_INFO(renderer.getDevice()->CreateBuffer(&ibd, &isd, &m_indexBuffer));

}

void Graphics::createShaders(Renderer& renderer) {
	ifstream vsFile("shaders/triangleVertexShader.cso", ios::binary); //inputfilestream
	vector<char> vsData = { istreambuf_iterator<char>(vsFile), istreambuf_iterator<char>() }; //creating iterator, and add byte by byte to vector, pretty slow, can be faster and more efficient.

	GFX_THROW_INFO(renderer.getDevice()->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &m_vertexShader));


	ifstream psFile("shaders/trianglePixelShader.cso", ios::binary);
	vector<char> psData = { istreambuf_iterator<char>(psFile), istreambuf_iterator<char>() };

	GFX_THROW_INFO(renderer.getDevice()->CreatePixelShader(psData.data(), psData.size(), nullptr, &m_pixelShader));


	//Create input (vertex) layouts
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXTCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	GFX_THROW_INFO(renderer.getDevice()->CreateInputLayout(
		layout, (UINT)std::size(layout),
		vsData.data(), 
		vsData.size(), 
		&m_inputLayout));

}

void Graphics::createRenderStates(Renderer& renderer) {
	// Rasterizer state
	auto rasterizerDesc = CD3D11_RASTERIZER_DESC(
		D3D11_FILL_SOLID,
		D3D11_CULL_FRONT, //draw only visible back shapes
		//D3D11_CULL_NONE, //draw all shapes
		false,
		0, 0, 0,
		false, false, false, false);
	GFX_THROW_INFO(renderer.getDevice()->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState));

	// Blend state
	auto blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	GFX_THROW_INFO(renderer.getDevice()->CreateBlendState(&blendDesc, &m_blendState));

	// Depth state
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	
	GFX_THROW_INFO(renderer.getDevice()->CreateDepthStencilState(&dsDesc, &m_depthState));

}