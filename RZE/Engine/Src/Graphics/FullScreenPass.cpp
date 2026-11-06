#include <Graphics/FullScreenPass.h>

#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

void DrawFullScreenPass(const TypedResourceHandle<PixelShader>& shader, std::initializer_list<Rendering::TextureBuffer2DHandle> inputs)
{
	Rendering::Renderer::SetPixelShader(shader->GetPlatformObject());

	U32 slot = 0;
	for (const Rendering::TextureBuffer2DHandle& input : inputs)
	{
		Rendering::Renderer::SetTextureResource(input, slot);
		++slot;
	}

	Rendering::Renderer::DrawFullScreenQuad();

	for (U32 unbound = 0; unbound < slot; ++unbound)
	{
		Rendering::Renderer::UnsetTextureResource(unbound);
	}
}
