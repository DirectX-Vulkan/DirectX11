#include "Renderer.h"
#include "Graphics.h"
#include "dxerr.h"
#include <sstream>


D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;


#pragma region d3d11debug
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


Renderer::Renderer(Window& window) {
	createDevice(window);
	createRenderTarget();
	createStensilState();
}

//destructor
Renderer::~Renderer() {
	m_device->Release();
	m_deviceContext->Release();
	m_renderTargetView->Release();
	m_swapChain->Release();
}

HRESULT Renderer::createDevice(Window& window) {
	HRESULT hr;

	// Define our swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = window.getHandle();
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0; //fps; but ignored becaus is windowed
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;;
	swapChainDesc.Windowed = true;

	UINT swapCreateFlags = 0u;
#ifndef NDEBUG
	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
	swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	
	// Create the swap chain, device and device context
	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, swapCreateFlags,
		nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc, &m_swapChain,
		&m_device, nullptr, &m_deviceContext));

	return hr;
}

void Renderer::createRenderTarget() {
	HRESULT hr = S_OK;
	ID3D11Texture2D* backBuffer;
	GFX_THROW_INFO(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &backBuffer));

	GFX_THROW_INFO(m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView));

	backBuffer->GetDesc(&m_backBufferDesc);
	backBuffer->Release();
	return;
}

void Renderer::createStensilState() {
	HRESULT hr = S_OK;

	// create depth stensil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = (float)m_backBufferDesc.Width; //------------------------------------------------------------------------------------
	descDepth.Height = (float)m_backBufferDesc.Height;
	descDepth.MipLevels = 1u; //mipmapping, just 1 mip map level
	descDepth.ArraySize = 1u; //we only want a single texture
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1u; 
	descDepth.SampleDesc.Quality = 0u; //anti alliasing setting
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	GFX_THROW_INFO(m_device->CreateTexture2D(&descDepth, nullptr, &m_depthStencilTexture));

	// create view of depth stensil texture
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0u;
	GFX_THROW_INFO(m_device->CreateDepthStencilView(m_depthStencilTexture, &descDSV, &m_depthStencilView));

	// bind depth stensil view to OM
	m_deviceContext->OMSetRenderTargets(1u, &m_renderTargetView, m_depthStencilView);

	return;
}

void Renderer::beginFrame(float red, float green, float blue) {

	// Set the background color
	const float clearColor[] = { .25f, .5f, 1, 1 };
	const float black[] = { 0, 0, 0, 1 };
	const float color[] = { red,green,blue,1.0f };
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0u);

	m_deviceContext->ClearRenderTargetView(m_renderTargetView, black);
	//m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// Bind render target
	m_deviceContext->OMSetRenderTargets(1u, &m_renderTargetView, nullptr); //Output merger

	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //What to do with the vertices points

	// Set viewport
	auto viewport = CD3D11_VIEWPORT(0.f, 0.f, (float)m_backBufferDesc.Width, (float)m_backBufferDesc.Height); //can be an array of multiple viewports
	m_deviceContext->RSSetViewports(1, &viewport);
}

void Renderer::endFrame() {

	HRESULT hr;
#ifndef NDEBUG
	infoManager.Set();
#endif
	// Swap the buffer!
	if (FAILED(hr = m_swapChain->Present(0u, 0u)))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
			throw GFX_DEVICE_REMOVED_EXCEPT(m_device->GetDeviceRemovedReason()); //device removed can occur on driver crashes, or when you remove the device ;)
		else
			throw GFX_EXCEPT(hr);
	}
}

ID3D11Device* Renderer::getDevice() {
	return m_device;
}

ID3D11DeviceContext* Renderer::getDeviceContext() {
	return m_deviceContext;
}

// Graphics exception stuff
Renderer::HrException::HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs) noexcept:Exception(line, file), hr(hr)
{
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs)
	{
		info += m;
		info.push_back('\n');
	}
	// remove final newline if exists
	if (!info.empty())
		info.pop_back();
}

const char* Renderer::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Error String] " << GetErrorString() << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl;
	if (!info.empty())
	{
		oss << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
	}
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Renderer::HrException::GetType() const noexcept
{
	return "Chili Graphics Exception";
}

HRESULT Renderer::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string Renderer::HrException::GetErrorString() const noexcept
{
	return DXGetErrorString(hr);
}

std::string Renderer::HrException::GetErrorDescription() const noexcept
{
	char buf[512];
	DXGetErrorDescription(hr, buf, sizeof(buf));
	return buf;
}

std::string Renderer::HrException::GetErrorInfo() const noexcept
{
	return info;
}


const char* Renderer::DeviceRemovedException::GetType() const noexcept
{
	return "Chili Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}