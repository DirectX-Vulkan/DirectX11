#pragma once

#include "ChiliWin.h"
#include "DxgiInfoManager.h"
#include "Window.h"
#include <d3d11.h>

class Renderer {
public:
	Renderer(Window& window);
	~Renderer(); //destructor
	void beginFrame(float red, float green, float blue);
	void endFrame();
	ID3D11Device* getDevice();
	ID3D11DeviceContext* getDeviceContext();

	class Exception : public ChiliException
	{
		using ChiliException::ChiliException;
	};
	class HrException : public Exception
	{
	public:
		HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
		std::string GetErrorDescription() const noexcept;
		std::string GetErrorInfo() const noexcept;
	private:
		HRESULT hr;
		std::string info;
	};
	class DeviceRemovedException : public HrException
	{
		using HrException::HrException;
	public:
		const char* GetType() const noexcept override;
	private:
		std::string reason;
	};

private:
	HRESULT createDevice(Window& window);
	void createRenderTarget();
	void createStensilState();

	// Device stuff
	IDXGISwapChain* m_swapChain = nullptr;
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;

	// Render target
	ID3D11RenderTargetView* m_renderTargetView = nullptr;

	D3D11_TEXTURE2D_DESC m_backBufferDesc;

	// Stencil stuff
	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11Texture2D* m_depthStencilTexture = nullptr;
	ID3D11DepthStencilView* m_depthStencilView = nullptr;

#ifndef NDEBUG
	DxgiInfoManager infoManager;
#endif

};