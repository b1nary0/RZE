#include <Diotima/Renderer.h>

#include <Optick/optick.h>

#include <Diotima/Graphics/GFXPassGraph.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Conversions.h>
#include <Utils/MemoryUtils.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/DebugUtils/Debug.h>
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

namespace Diotima
{

	void Renderer::FwdPassShim(const std::vector<Diotima::RenderObject>& renderObjects)
	{
		ID3D11DeviceContext& deviceContext = mDevice->GetDeviceContext();
		for (const Diotima::RenderObject& renderObject : renderObjects)
		{
			// try to draw something
			mDevice->GetDeviceContext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			DX11GFXConstantBuffer* modelMatBuf = mDevice->GetConstantBuffer(renderObject.ConstantBuffer);
			ID3D11Buffer* hwModelMatBuf = &modelMatBuf->GetHardwareBuffer();
			deviceContext.VSSetConstantBuffers(1, 1, &hwModelMatBuf);

			DX11GFXConstantBuffer* materialBuf = mDevice->GetConstantBuffer(renderObject.MaterialBuffer);
			ID3D11Buffer* hwMaterialBuf = &materialBuf->GetHardwareBuffer();
			deviceContext.PSSetConstantBuffers(2, 1, &hwMaterialBuf);

			ID3D11Buffer* vertBuf = &mDevice->GetVertexBuffer(renderObject.VertexBuffer)->GetHardwareBuffer();

			struct TempDataLayoutStructure
			{
				Vector3D position;
				Vector3D normal;
				Vector2D uv;
				Vector3D tangents;
			};
			UINT stride = sizeof(TempDataLayoutStructure);
			UINT offset = 0;
			mDevice->GetDeviceContext().IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);

			// Index buffer
			DX11GFXIndexBuffer* indexBuf = mDevice->GetIndexBuffer(renderObject.IndexBuffer);
			mDevice->GetDeviceContext().IASetIndexBuffer(&indexBuf->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

#pragma region TEXTURES
// 			if (drawCall.IsTextured)
// 			{
// 				std::array<DX11GFXTextureBuffer2D*, 3> textureArray = {
// 									mDevice->GetTextureBuffer2D(drawCall.TextureSlot0),
// 									mDevice->GetTextureBuffer2D(drawCall.TextureSlot1),
// 									mDevice->GetTextureBuffer2D(drawCall.TextureSlot2)
// 				};
// 
// 				for (size_t texBufIdx = 0; texBufIdx < textureArray.size(); ++texBufIdx)
// 				{
// 					ID3D11ShaderResourceView* resourceView = &textureArray[texBufIdx]->GetResourceView();
// 					ID3D11SamplerState* samplerState = &textureArray[texBufIdx]->GetSamplerState();
// 					deviceContext.PSSetShaderResources(texBufIdx, 1, &resourceView);
// 					deviceContext.PSSetSamplers(texBufIdx, 1, &samplerState);
// 				}
// 			}
#pragma endregion

			deviceContext.DrawIndexed(indexBuf->GetIndexCount(), 0, 0);
		}
	}

	Renderer::Renderer()
		: mRenderTarget(nullptr)
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Initialize()
	{
		mCanvasSize.SetXY(1600, 900);

		mDevice = std::make_unique<DX11GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();

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
		if (mRenderTarget != nullptr)
		{
			ID3D11RenderTargetView* nullViews[] = { nullptr };
			mDevice->GetDeviceContext().OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);

			mDevice->GetDeviceContext().OMSetRenderTargets(1, &mDevice->mRenderTargetView, mDevice->mDepthStencilView);

			FLOAT rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			mDevice->GetDeviceContext().ClearRenderTargetView(mDevice->mRenderTargetView, rgba);
		}

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


	Diotima::RenderObjectHandle Renderer::CreateRenderObject()
	{
		RenderObjectHandle objectHandle;

		if (!mFreeRenderObjectIndices.empty())
		{
			U32 freeIndex = mFreeRenderObjectIndices.front();
			mFreeRenderObjectIndices.pop();
			objectHandle.Value = freeIndex;

			RenderObject& renderObject = mRenderObjects[freeIndex];
			renderObject = {};

			return objectHandle;
		}

		objectHandle.Value = mRenderObjects.size();
		mRenderObjects.emplace_back();

		return objectHandle;
	}

	void Renderer::SetVertexBuffer(const RenderObjectHandle& handle, U32 buffer)
	{
		AssertMsg(handle.Value >= 0, "Invalid RenderObjectHandle.");
		mRenderObjects[handle.Value].VertexBuffer = buffer;
	}

	void Renderer::SetIndexBuffer(const RenderObjectHandle& handle, U32 buffer)
	{
		AssertMsg(handle.Value >= 0, "Invalid RenderObjectHandle.");
		mRenderObjects[handle.Value].IndexBuffer = buffer;
	}

	void Renderer::ProcessCommands()
	{
		OPTICK_EVENT();
		
	}

}
