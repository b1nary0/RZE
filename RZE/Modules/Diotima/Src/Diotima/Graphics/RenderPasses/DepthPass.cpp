#include <Diotima/Graphics/RenderPasses/DepthPass.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#include <Optick/optick.h>

namespace Diotima
{

	DepthPass::DepthPass()
		: mDevice(nullptr)
	{
	}

	DepthPass::~DepthPass()
	{
	}

	void DepthPass::Initialize(int width, int height)
	{
	}

	void DepthPass::Begin()
	{
	}

	void DepthPass::Execute()
	{
	
	}

	void DepthPass::End()
	{
	
	}

	void DepthPass::SetRenderer(Renderer* renderer)
	{
		mRenderer = renderer;
	}

	void DepthPass::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		AssertExpr(mDevice == nullptr);

		mDevice = device;
	}

	void DepthPass::PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights)
	{
		//OPTICK_EVENT();

// 		DX12GFXConstantBuffer* const lightConstantBuffer = mDevice->GetConstantBuffer(mLightConstantBuffer);
// 
// 		lightConstantBuffer->Reset();
// 
// 		AssertMsg(lights.size() == 1, "First pass shadows only support one light"); 
// 		lightConstantBuffer->AllocateMember(&lights[0].LightSpaceMatrix);
	}

}

