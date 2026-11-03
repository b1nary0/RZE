#include <StdAfx.h>
#include <Rendering/RenderThread.h>

#include <Rendering/RenderCommand.h>
#include <Rendering/UIFrameData.h>

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/ImGui/ImGuiRenderer.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/TimeUtils.h>

#include <Optick/optick.h>

// What each render command does on the render thread: RenderThread::ProcessCommands calls the Execute for its type.
// What touches the graphics API is the device's (IGFXDevice) or the resource's.
namespace Rendering
{
	void RenderThread::Execute(RenderCommand_ImGuiRender& command)
	{
		// The copy is deleted when the command lets go of it, unless Renderer still holds it for the next frame
		m_imguiRenderer->RenderDrawData(command.uiFrameData->GetDrawData());
	}

	void RenderThread::Execute(RenderCommand_BeginFrame& command)
	{
		BeginProfilerEvent(command.frameName);
	}

	void RenderThread::Execute(RenderCommand_EndFrame&)
	{
		EndProfilerEvent();
	}

	void RenderThread::Execute(RenderCommand_Begin& command)
	{
		BeginProfilerEvent(command.drawSetName);

		m_device->BeginDrawSet();
	}

	void RenderThread::Execute(RenderCommand_End&)
	{
		// #TODO form the api such that we can verify Begin() and End() calls for sanity checks
		EndProfilerEvent();
	}

	void RenderThread::Execute(RenderCommand_DevicePresent&)
	{
		OPTICK_EVENT("Device Present");
		const double presentStart = TimeUtils::GetSeconds();
		m_device->Present();
		m_framePresentTime += TimeUtils::GetSeconds() - presentStart;
	}

	void RenderThread::Execute(RenderCommand_HandleWindowResize& command)
	{
		const Vector2D& newSize = command.size;
		m_device->HandleWindowResize(static_cast<U32>(newSize.X()), static_cast<U32>(newSize.Y()));
	}

	void RenderThread::Execute(RenderCommand_SetSyncInterval& command)
	{
		m_device->SetSyncInterval(command.interval);
	}

	void RenderThread::Execute(RenderCommand_CreateVertexBuffer& command)
	{
		command.bufferPtr->SetDynamic(command.isDynamic);
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count, command.stride);
	}

	void RenderThread::Execute(RenderCommand_CreateIndexBuffer& command)
	{
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count);
	}

	void RenderThread::Execute(RenderCommand_CreateConstantBuffer& command)
	{
		command.bufferPtr->Allocate(command.data, command.dataTypeSize, command.count);
	}

	void RenderThread::Execute(RenderCommand_CreateTextureBuffer2D& command)
	{
		command.bufferPtr->Allocate(command.texels.GetTexels(), command.params);
	}

	void RenderThread::Execute(RenderCommand_CreateVertexShader& command)
	{
		command.bufferPtr->Create(command.filepath, command.shaderInputLayout);
	}

	void RenderThread::Execute(RenderCommand_CreatePixelShader& command)
	{
		command.bufferPtr->Create(command.filepath);
	}

	void RenderThread::Execute(RenderCommand_CreateComputeShader& command)
	{
		command.bufferPtr->Create(command.filepath);
	}

	void RenderThread::Execute(RenderCommand_UploadDataToBuffer& command)
	{
		command.bufferHandle.m_buffer->UpdateData(command.data);
	}

	void RenderThread::Execute(RenderCommand_UpdateVertexBuffer& command)
	{
		command.bufferHandle.m_buffer->UpdateData(command.data, command.size);
	}

	void RenderThread::Execute(RenderCommand_ReleaseVertexShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ReleasePixelShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ReleaseComputeShader& command)
	{
		command.shaderHandle.m_shader.reset();
	}

	void RenderThread::Execute(RenderCommand_ClearDepthStencilBuffer& command)
	{
		m_device->ClearDepthStencilBuffer(*command.bufferHandle.m_buffer, command.bEverySlice, command.slice);
	}

	void RenderThread::Execute(RenderCommand_ClearRenderTargets&)
	{
		m_device->ClearRenderTargets();
	}

	void RenderThread::Execute(RenderCommand_SetRenderTargetBackBuffer&)
	{
		m_device->SetRenderTargetBackBuffer();
	}

	void RenderThread::Execute(RenderCommand_SetDepthTarget& command)
	{
		m_device->SetDepthTarget(*command.depthTexture.m_buffer, command.slice);
	}

	void RenderThread::Execute(RenderCommand_SetColourTarget& command)
	{
		m_device->SetColourTarget(*command.colourTarget.m_buffer, command.hasDepth ? command.depthTexture.m_buffer.get() : nullptr);
	}

	void RenderThread::Execute(RenderCommand_GenerateMips& command)
	{
		m_device->GenerateMips(*command.texture.m_buffer);
	}

	void RenderThread::Execute(RenderCommand_CopyTexture& command)
	{
		m_device->CopyTexture(*command.source.m_buffer, *command.destination.m_buffer);
	}

	void RenderThread::Execute(RenderCommand_CopyTextureRegion& command)
	{
		m_device->CopyTextureRegion(*command.source.m_buffer, command.sourceX, command.sourceY, command.width, command.height,
			*command.destination.m_buffer, command.destinationX, command.destinationY);
	}

	void RenderThread::Execute(RenderCommand_ClearRenderTarget& command)
	{
		m_device->ClearRenderTarget(*command.renderTarget.m_buffer, command.colour);
	}

	void RenderThread::Execute(RenderCommand_SetViewport& command)
	{
		m_device->SetViewport(command.params);
	}

	void RenderThread::Execute(RenderCommand_SetInputLayout& command)
	{
		command.shaderHandle.m_shader->SetInputLayout();
	}

	void RenderThread::Execute(RenderCommand_SetPrimitiveTopology& command)
	{
		m_device->SetPrimitiveTopology(command.topology);
	}

	void RenderThread::Execute(RenderCommand_SetRasterizerState& command)
	{
		m_device->SetRasterizerState(command.state);
	}

	void RenderThread::Execute(RenderCommand_SetBlendState& command)
	{
		m_device->SetBlendState(command.state);
	}

	void RenderThread::Execute(RenderCommand_SetDepthState& command)
	{
		m_device->SetDepthState(command.state);
	}

	void RenderThread::Execute(RenderCommand_SetVertexShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetPixelShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_UnsetPixelShader&)
	{
		m_device->UnsetPixelShader();
	}

	void RenderThread::Execute(RenderCommand_SetComputeShader& command)
	{
		command.shaderHandle.m_shader->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferVS& command)
	{
		command.bufferHandle.m_buffer->SetActiveVS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferPS& command)
	{
		command.bufferHandle.m_buffer->SetActivePS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetConstantBufferCS& command)
	{
		command.bufferHandle.m_buffer->SetActiveCS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetVertexBuffer& command)
	{
		command.bufferHandle.m_buffer->SetActive(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_SetIndexBuffer& command)
	{
		command.bufferHandle.m_buffer->SetActive();
	}

	void RenderThread::Execute(RenderCommand_SetTextureResource& command)
	{
		command.bufferHandle.m_buffer->SetActive(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_UnsetTextureResource& command)
	{
		m_device->UnsetTextureResource(command.textureSlot);
	}

	void RenderThread::Execute(RenderCommand_SetTextureResourceCS& command)
	{
		command.bufferHandle.m_buffer->SetActiveCS(command.bufferSlot);
	}

	void RenderThread::Execute(RenderCommand_UnsetTextureResourceCS& command)
	{
		m_device->UnsetTextureResourceCS(command.textureSlot);
	}

	void RenderThread::Execute(RenderCommand_SetUnorderedAccess& command)
	{
		m_device->SetUnorderedAccess(*command.texture.m_buffer, command.slot, command.mip);
	}

	void RenderThread::Execute(RenderCommand_UnsetUnorderedAccess& command)
	{
		m_device->UnsetUnorderedAccess(command.slot);
	}

	void RenderThread::Execute(RenderCommand_Draw& command)
	{
		m_device->Draw(static_cast<U32>(command.count));
	}

	void RenderThread::Execute(RenderCommand_DrawIndexed& command)
	{
		m_device->DrawIndexed(command.indexBuffer.m_buffer->GetIndexCount());
	}

	void RenderThread::Execute(RenderCommand_DrawFullScreenQuad&)
	{
		m_device->Draw(4);
	}

	void RenderThread::Execute(RenderCommand_Dispatch& command)
	{
		m_device->Dispatch(command.groupsX, command.groupsY, command.groupsZ);
	}

	void RenderThread::Execute(RenderCommand_ReadTexel& command)
	{
		m_device->ReadTexel(*command.texture.m_buffer, command.x, command.y, *command.result);
	}

	void RenderThread::Execute(RenderCommand_ReadTexture& command)
	{
		m_device->ReadTexture(*command.texture.m_buffer, *command.result);
	}
}
