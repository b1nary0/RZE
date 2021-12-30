#include <Diotima/Renderer.h>

#include <Optick/optick.h>

#include <Diotima/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Memory/MemoryUtils.h>
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
		Diotima::IGFXConstantBuffer* mCameraDataBuffer = nullptr;

		// #TODO
		// Temp struct just to move code.
		CameraGPUData mCameraGPUData;

		void Initialize(Diotima::DX11GFXDevice* hwDevice)
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

namespace Diotima
{
	static DrawStateData_Prototype kDrawStateData;

	Renderer::Renderer()
		: mRenderTarget(nullptr)
		, mWindowHandle(nullptr)
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::PrepareDrawState()
	{
		OPTICK_EVENT();

		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();

		deviceContext.RSSetState(mDevice->mRasterState);
		deviceContext.VSSetShader(kDrawStateData.mVertexShader, 0, 0);
		
		kDrawStateData.mCameraGPUData.ClipSpace = mCameraData.ProjectionMat * mCameraData.ViewMat;
		kDrawStateData.mCameraGPUData.Position = mCameraData.Position;

		DX11GFXConstantBuffer* camDataBuf = static_cast<DX11GFXConstantBuffer*>(kDrawStateData.mCameraDataBuffer);
		ID3D11Buffer* vpbHardwareBuf = &camDataBuf->GetHardwareBuffer();
		camDataBuf->UpdateSubresources(&kDrawStateData.mCameraGPUData);
		deviceContext.VSSetConstantBuffers(0, 1, &vpbHardwareBuf);

		// This is early code supporting a single proxy render target.
		ID3D11RenderTargetView* targetViews[1] = { nullptr };
		ID3D11DepthStencilView* targetDepthView = nullptr;
		if (mRenderTarget != nullptr)
		{
			targetViews[0] = &mRenderTarget->GetGFXTexture().GetTargetView();
			targetDepthView = &mRenderTarget->GetDepthTexture().GetDepthView();
		}
		else
		{
			targetViews[0] = { mDevice->mRenderTargetView };
			targetDepthView = mDevice->mDepthStencilView;
		}

		deviceContext.OMSetRenderTargets(1, targetViews, targetDepthView);

		deviceContext.ClearRenderTargetView(targetViews[0], rgba);
		deviceContext.ClearDepthStencilView(targetDepthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		D3D11_VIEWPORT viewport;
		viewport.Width = GetViewportSize().X();
		viewport.Height = GetViewportSize().Y();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		deviceContext.RSSetViewports(1, &viewport);

		deviceContext.IASetInputLayout(kDrawStateData.mVertexLayout);
		deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void Renderer::Draw()
	{
		OPTICK_EVENT();

		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();

		{
			OPTICK_EVENT("RenderObject Sort");

			std::sort(mRenderObjects.begin(), mRenderObjects.end(),
				[](const RenderObject* objA, const RenderObject* objB)
				{
					return objA->Material.mShaderID < objB->Material.mShaderID;
				}
			);
		}

		for (auto& renderObject : mRenderObjects)
		{
			ID3D11PixelShader* const hwShader = mDevice->GetPixelShader(renderObject->Material.mShaderID);
			deviceContext.PSSetShader(hwShader, 0, 0);

			//
			// Shader State
			{
				{
					// #TODO
					// This is god awful. Just in place while developing shader model.
					// Should get resolved once the system matures
					DX11GFXConstantBuffer* cameraDataBuf = static_cast<DX11GFXConstantBuffer*>(kDrawStateData.mCameraDataBuffer);
					ID3D11Buffer* camDataHWbuf = &cameraDataBuf->GetHardwareBuffer();
					deviceContext.PSSetConstantBuffers(0, 1, &camDataHWbuf);
				}

				// World Matrix
				{
					DX11GFXConstantBuffer* modelMatBuf = static_cast<DX11GFXConstantBuffer*>(renderObject->ConstantBuffer);
					ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
					deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);
				}

				// Material buffer
				{
					DX11GFXConstantBuffer* materialBuffer = static_cast<DX11GFXConstantBuffer*>(renderObject->MaterialDataBuffer);
					ID3D11Buffer* matHWbuf = &materialBuffer->GetHardwareBuffer();
					materialBuffer->UpdateSubresources(&renderObject->Material.mProperties);
					deviceContext.PSSetConstantBuffers(1, 1, &matHWbuf);
				}
				// Textures
				{
					if (renderObject->Material.mTexturePack->GetResourceCount() > 0)
					{
						DX11GFXTextureBuffer2D* const textureBuf = renderObject->Material.mTexturePack->GetResourceAt(0);
						// #TODO
						// Move the sampler state elsewhere so we're not bound to an individual texture resource for it...
						ID3D11SamplerState* const samplerState = &textureBuf->GetSamplerState();

						std::vector<ID3D11ShaderResourceView*> resourceViews = renderObject->Material.mTexturePack->GetAsGPUTextureArray();
						deviceContext.PSSetShaderResources(0, resourceViews.size(), resourceViews.data());
						deviceContext.PSSetSamplers(0, 1, &samplerState);
					}
				}
			}
			
			//
			// Mesh Data
			{
				// Vertex buffer
				struct TempDataLayoutStructure
				{
					Vector3D position;
					Vector3D normal;
					Vector2D uv;
					Vector3D tangent;
				};
				UINT stride = sizeof(TempDataLayoutStructure);
				UINT offset = 0;

				// #TODO
				// Code like this needs to be removed by writing a proper abstraction layer that get provide this functionality
				// while still obfuscating the underlying type
				ID3D11Buffer* vertBuf = &static_cast<DX11GFXVertexBuffer*>(renderObject->VertexBuffer)->GetHardwareBuffer();
				mDevice->GetDeviceContext().IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);
			}

			// Index buffer
			DX11GFXIndexBuffer* indexBuf = static_cast<DX11GFXIndexBuffer*>(renderObject->IndexBuffer);
			mDevice->GetDeviceContext().IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

			// #TODO
			// When optimizing, ideally by this point we have collapsed all collapsable geometry possible to 
			// a single vertex buffer, so we can just have a single draw call.
			deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
		}

		mRenderObjects.clear();

		// #TODO
		// Have a better place for this. Kind of hidden at the end of Draw() here
		// This is early code supporting a single proxy render target.
		if (mRenderTarget != nullptr)
		{
			deviceContext.OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

			deviceContext.ClearRenderTargetView(mDevice->mRenderTargetView, rgba);
			deviceContext.ClearDepthStencilView(mDevice->mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	void Renderer::SetCameraData(const CameraData& cameraData)
	{
		mCameraData = cameraData;
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
		OPTICK_EVENT();

		PrepareDrawState();
		Draw();

		{
			OPTICK_EVENT("ImGui Render");
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

		{
			OPTICK_EVENT("Present");
			mDevice->Present();
		}
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

		mDevice->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));

		// This code is kinda wonky in that if you end up in a situation where you set the viewport
		// then resize the window, you'll need to re-set your viewport. i will need to address this in the future.
		SetViewportSize(newSize);

		LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", mCanvasSize.X(), mCanvasSize.Y());
	}

	void Renderer::SetViewportSize(const Vector2D& newSize)
	{
		mViewportDimensions = newSize;
	}

	const Vector2D& Renderer::GetViewportSize()
	{
		return mViewportDimensions;
	}

	void Renderer::SetRenderTarget(RenderTargetTexture* renderTarget)
	{
		//#TODO_CRITICAL
		// Dont initialize here... just did this for ease of access to the device for PoC
		renderTarget->Initialize(mDevice.get());

		mRenderTarget = renderTarget;
	}

	IGFXVertexBuffer* Renderer::CreateVertexBuffer(void* data, size_t size, U32 count)
	{
		return mDevice->CreateVertexBuffer(data, size, count);
	}

	IGFXIndexBuffer* Renderer::CreateIndexBuffer(void* data, size_t size, U32 count)
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

	Int32 Renderer::CreatePixelShader(const FilePath& filePath)
	{
		return mDevice->CreatePixelShader(filePath);
	}

	RenderObject* Renderer::CreateAndInitializeRenderObject(
		const MeshData& meshData, 
		const std::vector<TextureData>& textureData, 
		const Matrix4x4& transform)
	{
		RenderObject* renderObject = new RenderObject();
		InitializeRenderObject(*renderObject, meshData, textureData, transform);

		return renderObject;
	}

	void Renderer::DestroyRenderObject(RenderObject* renderObject)
	{	
		renderObject->VertexBuffer->Release();
		renderObject->IndexBuffer->Release();
		renderObject->ConstantBuffer->Release();
		renderObject->MaterialDataBuffer->Release();

		renderObject->Material.mTexturePack->Release();

		delete renderObject;
		renderObject = nullptr;
	}

	void Renderer::InitializeRenderObject(
		RenderObject& renderObject, 
		const MeshData& meshData, 
		const std::vector<TextureData>& textureData,
		const Matrix4x4& transform)
	{
		renderObject.Transform = transform;
		renderObject.VertexBuffer = CreateVertexBuffer((void*)meshData.Vertices.data(), sizeof(float), meshData.Vertices.size());
		renderObject.IndexBuffer = CreateIndexBuffer((void*)meshData.Indices.data(), sizeof(U32), meshData.Indices.size());
		renderObject.ConstantBuffer = mDevice->CreateConstantBuffer(sizeof(Matrix4x4) * 2, 1);
		renderObject.MaterialDataBuffer = mDevice->CreateConstantBuffer(MemoryUtils::AlignSize(sizeof(MaterialData::MaterialProperties), 16), 1);

		{
			// Material Setup
			renderObject.Material = meshData.Material;
			renderObject.Material.mTexturePack = new TexturePack();
			renderObject.Material.mTexturePack->Allocate(*mDevice, textureData);
		}

		renderObject.bEnabled = true;

		UpdateRenderObjectTransform_GPU(renderObject);
	}

	void Renderer::UpdateRenderObject(RenderObject* renderObject, const Matrix4x4& newTransform)
	{
		renderObject->Transform = newTransform;
		UpdateRenderObjectTransform_GPU(*renderObject);
	}

	void Renderer::AddRenderObject(RenderObject* renderObject)
	{
		// #TODO
		// handle dupe entries somehow - at least assert
		mRenderObjects.push_back(renderObject);
	}

	void Renderer::UpdateRenderObjectTransform_GPU(const RenderObject& renderObject)
	{
		DX11GFXConstantBuffer* constBuf = static_cast<DX11GFXConstantBuffer*>(renderObject.ConstantBuffer);
		struct MatrixMem
		{
			Matrix4x4 world;
			Matrix4x4 inverseWorld;
		} matrixMem;

		matrixMem.world = renderObject.Transform;
		matrixMem.inverseWorld = renderObject.Transform.Inverse();

		constBuf->UpdateSubresources(&matrixMem);
	}

	void Renderer::ProcessCommands()
	{
		OPTICK_EVENT();
	}

}
