#include <Diotima/Graphics/RenderPasses/ImGUIPass.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx12.h>

namespace Diotima
{

	ImGUIPass::ImGUIPass()
	{
	}

	ImGUIPass::~ImGUIPass()
	{
	}

	void ImGUIPass::Initialize(int width, int height)
	{
		//SetDevice(mRenderer->mDX12Device.get());
// 		mCommandList = mDevice->CreateGraphicsCommandList(mDevice->GetCommandAllocator(), nullptr);
// 		mDevice->GetGraphicsCommandList(mCommandList)->Close();
// 
// 		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
// 		srvHeapDesc.NumDescriptors = 1024;
// 		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
// 		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
// 		mDevice->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mTextureHeap));
// 
// 		ImGui::CreateContext();
// 
// 		ImGui_ImplDX12_Init(
// 			mDevice->GetDevice(),
// 			2,
// 			DXGI_FORMAT_R8G8B8A8_UNORM,
// 			mTextureHeap->GetCPUDescriptorHandleForHeapStart(),
// 			mTextureHeap->GetGPUDescriptorHandleForHeapStart());
// 
// 		OnWindowResize(width, height);
	}

	void ImGUIPass::Begin()
	{
	}

	void ImGUIPass::Execute()
	{
// 		OPTICK_EVENT();
// 
// 		ID3D12GraphicsCommandList* const commandList = mDevice->GetGraphicsCommandList(mCommandList);
// 		commandList->Reset(mDevice->GetCommandAllocator(), nullptr);
// 
// 		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mDevice->GetRenderTargetHeap()->GetCPUDescriptorHandleForHeapStart(), mDevice->GetCurrentFrame(), mDevice->GetRTVDescriptorSize());
// 		const float clearColor[] = { 0.f, 0.f, 0.f, 0.f };
// 
// 		D3D12_RESOURCE_BARRIER barrier = {};
// 		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
// 		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
// 		barrier.Transition.pResource = mDevice->GetCurrentFrameRenderTarget();
// 		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
// 		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
// 		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
// 
// 		commandList->ResourceBarrier(1, &barrier);
// 		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
// 
// 		ID3D12DescriptorHeap* ppDescHeaps[] = { mTextureHeap };
// 		commandList->SetDescriptorHeaps(_countof(ppDescHeaps), ppDescHeaps);
// 
// 		ImGui::Render();
// 		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
// 		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
// 		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
// 		commandList->ResourceBarrier(1, &barrier);
// 
// 		mDevice->ExecuteCommandList(commandList);
	}

	void ImGUIPass::OnWindowResize(int newWidth, int newHeight)
	{
		ImGui::GetIO().DisplaySize.x = static_cast<float>(newWidth);
		ImGui::GetIO().DisplaySize.y = static_cast<float>(newHeight);
	}

	void ImGUIPass::End()
	{
	}

	void ImGUIPass::SetRenderer(Renderer* renderer)
	{
		AssertNotNull(renderer);
		mRenderer = renderer;
	}

	void ImGUIPass::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

}
