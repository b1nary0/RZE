#include <Diotima/Renderer.h>

#include <Optick/optick.h>

#include <Diotima/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Conversions.h>
#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

// DX11
#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Driver/DX11/DX11.h>
// #TODO(What makes this needed here? unique_ptr complains re: destructor visibility
//       is it necessary otherwise?)
#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Diotima/Graphics/RenderTarget.h>


#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include <array>

namespace
{
	D3D11_INPUT_ELEMENT_DESC k_vertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numLayoutElements = ARRAYSIZE(k_vertLayout);

	// [newrenderer]
	// Only used to test preliminary changes to how the renderer functions without
	// having to write too much code that won't last
	struct DrawStateData_Prototype
	{
		ID3D10Blob* mVSBlob = nullptr;
		ID3D10Blob* mPSBlob = nullptr;
		ID3D11VertexShader* mVertexShader = nullptr;
		ID3D11PixelShader* mPixelShader = nullptr;
		ID3D11InputLayout* mVertexLayout = nullptr;
		Int32 mViewProjBuffer = -1;

		void Initialize(Diotima::DX11GFXDevice* hwDevice)
		{
			mViewProjBuffer = hwDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);

			// Shaders
			{
				FilePath vertexShaderPath("Assets/Shaders/Vertex_NewRenderer.hlsl");
				FilePath pixelShaderPath("Assets/Shaders/Pixel_NewRenderer.hlsl");

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

				hr = D3DCompileFromFile(Conversions::StringToWString(pixelShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", 0, 0, &mPSBlob, &error);
				errorCB(error);

				hr = hwDevice->GetHardwareDevice().CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
				hr = hwDevice->GetHardwareDevice().CreatePixelShader(mPSBlob->GetBufferPointer(), mPSBlob->GetBufferSize(), nullptr, &mPixelShader);

				hr = hwDevice->GetHardwareDevice().CreateInputLayout(k_vertLayout, numLayoutElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);
			}
		}
	};
}

namespace Diotima
{
	static DrawStateData_Prototype kDrawStateData;

	Renderer::Renderer()
		: mRenderTarget(nullptr)
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::PrepareDrawState()
	{
		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();

		deviceContext.RSSetState(mDevice->mRasterState);
		deviceContext.VSSetShader(kDrawStateData.mVertexShader, 0, 0);
		deviceContext.PSSetShader(kDrawStateData.mPixelShader, 0, 0);

		Matrix4x4 camViewProjMat = mCameraData.ProjectionMat * mCameraData.ViewMat;
		DX11GFXConstantBuffer* viewProjBuf = mDevice->GetConstantBuffer(kDrawStateData.mViewProjBuffer);
		ID3D11Buffer* vpbHardwareBuf = &viewProjBuf->GetHardwareBuffer();
		viewProjBuf->UpdateSubresources(&camViewProjMat);
		deviceContext.VSSetConstantBuffers(0, 1, &vpbHardwareBuf);

		deviceContext.OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

		FLOAT rgba[4] = { 0.35f, 0.35f, 0.35f, 1.0f };

		deviceContext.ClearDepthStencilView(mDevice->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.ClearRenderTargetView(mDevice->mRenderTargetView, rgba);

		D3D11_VIEWPORT viewport;
		viewport.Width = GetCanvasSize().X();
		viewport.Height = GetCanvasSize().Y();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		deviceContext.RSSetViewports(1, &viewport);

		deviceContext.IASetInputLayout(kDrawStateData.mVertexLayout);
	}

	void Renderer::Draw()
	{
		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
		deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (auto& renderObject : mRenderObjects)
		{
			// World Matrix
			{
				DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(renderObject.ConstantBuffer);
				ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
				deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);
			}

			// Vertex buffer
			{
				struct TempDataLayoutStructure
				{
					Vector3D position;
					Vector3D normal;
				};
				UINT stride = sizeof(TempDataLayoutStructure);
				UINT offset = 0;

				ID3D11Buffer* vertBuf = &mDevice->GetVertexBuffer(renderObject.VertexBuffer)->GetHardwareBuffer();
				mDevice->GetDeviceContext().IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);
			}

			// Index buffer
			DX11GFXIndexBuffer* indexBuf = mDevice->GetIndexBuffer(renderObject.IndexBuffer);
			mDevice->GetDeviceContext().IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

			deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
		}
	}

	void Renderer::SetCameraData(
		const Vector3D& position, 
		const Matrix4x4& projectionMat, 
		const Matrix4x4& viewMat, 
		float FOV, 
		float aspectRatio, 
		float nearCull, 
		float farCull)
	{
		// lol gross
		mCameraData.Position = position;
		mCameraData.ProjectionMat = projectionMat;
		mCameraData.ViewMat = viewMat;
		mCameraData.FOV = FOV;
		mCameraData.AspectRatio = aspectRatio;
		mCameraData.NearCull = nearCull;
		mCameraData.FarCull = farCull;
	}

	void Renderer::Initialize()
	{
		mCanvasSize.SetXY(1600, 900);

		mDevice = std::make_unique<DX11GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();

		kDrawStateData.Initialize(mDevice.get());

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(mWindowHandle);
		ImGui_ImplDX11_Init(&mDevice->GetHardwareDevice(), &mDevice->GetDeviceContext());
	}

	void Renderer::Update()
	{
		OPTICK_EVENT();

		ProcessCommands();
	}

	void Renderer::Render()
	{
		PrepareDrawState();
		Draw();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		mDevice->Present();
	}

	void Renderer::ShutDown()
	{
		ImGui_ImplDX11_Shutdown();
		mDevice->Shutdown();
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		mDevice->SetSyncInterval(static_cast<U32>(bEnabled));
	}
	
	const Vector2D& Renderer::GetCanvasSize()
	{
		return mCanvasSize;
	}

	// #TODO
	// Take with new renderer
	void Renderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;

		ImGui::GetIO().DisplaySize.x = static_cast<float>(mCanvasSize.X());
		ImGui::GetIO().DisplaySize.y = static_cast<float>(mCanvasSize.Y());

		mDevice->HandleWindowResize(newSize);

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}

	// #TODO
	// Take with new renderer - only used in hacky RenderTarget code in EditorApp
	void Renderer::SetViewportSize(const Vector2D& newSize)
	{
		mViewportDimensions = newSize;
	}

	const Vector2D& Renderer::GetViewportSize()
	{
		return mViewportDimensions;
	}

	Int32 Renderer::CreateVertexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateVertexBuffer(data, size, count);
	}

	Int32 Renderer::CreateIndexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateIndexBuffer(data, size, count);
	}

	Int32 Renderer::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		// This is temp here, should be passed in or something.
		GFXTextureBufferParams params = { 0 };
		params.bIsRenderTarget = true;
		params.bIsShaderResource = true;
		params.Height = height;
		params.Width = width;
		params.MipLevels = 0;
		params.MostDetailedMip = 0;
		params.SampleCount = 1;
		params.SampleQuality = 0;
		return mDevice->CreateTextureBuffer2D(data, params);
	}

	U32 Renderer::CreateRenderObject(const MeshData& meshData, const Matrix4x4& transform)
	{
		if (!mFreeRenderObjectIndices.empty())
		{
			U32 freeIndex = mFreeRenderObjectIndices.front();
			mFreeRenderObjectIndices.pop();

			RenderObject& renderObject = mRenderObjects[freeIndex];
			renderObject = {};
			InitializeRenderObject(renderObject, meshData, transform);

			return freeIndex;
		}

		mRenderObjects.emplace_back();
		RenderObject& renderObject = mRenderObjects.back();
		InitializeRenderObject(renderObject, meshData, transform);

		return mRenderObjects.size() - 1;
	}

	void Renderer::InitializeRenderObject(RenderObject& renderObject, const MeshData& meshData, const Matrix4x4& transform)
	{
		renderObject.Transform = transform;
		renderObject.VertexBuffer = CreateVertexBuffer((void*)meshData.Vertices.data(), sizeof(float), meshData.Vertices.size());
		renderObject.IndexBuffer = CreateIndexBuffer((void*)meshData.Indices.data(), sizeof(U32), meshData.Indices.size());
		renderObject.ConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4), 1);

		{
			// Material Setup
			renderObject.Material = meshData.Material;
		}

		DX11GFXConstantBuffer* constBuf = mDevice->GetConstantBuffer(renderObject.ConstantBuffer);
		constBuf->UpdateSubresources(renderObject.Transform.GetValuePtr());
	}

	void Renderer::ProcessCommands()
	{
		OPTICK_EVENT();
	}

}
