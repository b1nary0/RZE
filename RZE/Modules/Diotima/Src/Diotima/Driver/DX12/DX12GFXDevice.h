#pragma once

#include <memory>

#include <wrl.h>

#include <Diotima/Driver/GFXDevice.h> 

#include <Utils/PrimitiveDefs.h>

using namespace Microsoft::WRL;

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;

struct IDXGIFactory4;
struct IDXGISwapChain3;

namespace Diotima
{
	// #TODO(Josh::Eventually move these into a more configurable/stateful place)
	constexpr int kBufferCount = 2;
	constexpr int kBufferWidth = 1920;
	constexpr int kBufferHeight = 1080;

	class DX12GFXDevice : public IGFXDevice
	{
		friend class DX12GFXDriverInterface;

	public:
		DX12GFXDevice();
		virtual ~DX12GFXDevice();

	public:
		virtual void Initialize() final override;
		// #TODO(Josh::Really really don't like this, fix later)
		virtual void SetWindow(void* windowHandle) final override;

	private:
		void InitializeAssets();

	private:
		ComPtr<ID3D12Device> mDevice;
		ComPtr<ID3D12CommandQueue> mCommandQueue;
		ComPtr<ID3D12CommandAllocator> mCommandAllocator;
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
		ComPtr<ID3D12Resource> mRenderTargets[kBufferCount];
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;

		ComPtr<IDXGIFactory4> mFactory;
		ComPtr<IDXGISwapChain3> mSwapChain;

		int mCurrentFrame;
		void* mWindowHandle;

		U32 mRTVDescriptorSize;
	};
}