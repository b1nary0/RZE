#include <Rendering/Renderer.h>

#include <Optick/optick.h>

#include <Rendering/Graphics/GFXPassGraph.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Rendering/Driver/DX11/DX11GFXDevice.h>
#include <Rendering/Driver/DX11/DX11.h>
// #TODO(What makes this needed here? unique_ptr complains re: destructor visibility
//       is it necessary otherwise?)
#include <Rendering/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include <array>

namespace
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numLayoutElements = ARRAYSIZE(k_vertLayout);

	FLOAT rgba[4] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// #TODO
	// Temp struct just to move code.
	struct CameraGPUData
	{
		Matrix4x4 ClipSpace;
		Vector3D Position;
	};

	// Only used to test preliminary changes to how the renderer functions without
	// having to write too much code that won't last
	struct DrawStateData_Prototype
	{
		ID3D10Blob* mVSBlob = nullptr;
		ID3D11VertexShader* mVertexShader = nullptr;
		ID3D11InputLayout* mVertexLayout = nullptr;
		Rendering::IGFXConstantBuffer* mCameraDataBuffer = nullptr;

		// #TODO
		// Temp struct just to move code.
		CameraGPUData mCameraGPUData;

		void Initialize(Rendering::DX11GFXDevice* hwDevice)
		{
			mCameraDataBuffer = hwDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(CameraGPUData), 16), 1);

			// Shaders
			{
				FilePath vertexShaderPath("Assets/Shaders/Vertex_NewRenderer.hlsl");

				HRESULT hr;
				ID3D10Blob* error;
				
				auto errorCB = [](ID3D10Blob* error)
				{
					if (error != nullptr)
					{
						char const* message =
							static_cast<char const*>(error->GetBufferPointer());

						// Write the warning to the output window when the program is
						// executing through the Microsoft Visual Studio IDE.
						size_t const length = strlen(message);
						std::wstring output = L"";
						for (size_t i = 0; i < length; ++i)
						{
							output += static_cast<wchar_t>(message[i]);
						}
						output += L'\n';
						OutputDebugString(output.c_str());
					}
				};

				hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", 0, 0, &mVSBlob, &error);
				errorCB(error);

				hr = hwDevice->GetHardwareDevice().CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
				hr = hwDevice->GetHardwareDevice().CreateInputLayout(k_vertLayout, numLayoutElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);
			}
		}
	};
}

namespace Rendering
{
	std::unique_ptr<DX11GFXDevice> Renderer::m_device;

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize(void* windowHandle)
	{
		m_device = std::make_unique<DX11GFXDevice>();
		m_device->SetWindow(windowHandle);
		m_device->Initialize();
		
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(windowHandle);
		ImGui_ImplDX11_Init(&m_device->GetHardwareDevice(), &m_device->GetDeviceContext());
	}
	
	void Renderer::ShutDown()
	{
		ImGui_ImplDX11_Shutdown();
		m_device->Shutdown();
	}
	
}
