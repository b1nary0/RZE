#include <StdAfx.h>
#include <Graphics/Material.h>

#include <Rendering/Renderer.h>

#include <Utils/Memory/MemoryUtils.h>

MaterialInstance::MaterialInstance()
{
	m_textureSlots.resize(TEXTURE_SLOT_COUNT);
	
	m_paramBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, MemoryUtils::AlignSize(sizeof(MaterialParams), 16), 1);
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::SetTexture(U8 textureSlot, const ResourceHandle& textureResource)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	m_textureSlots[textureSlot] = textureResource;
}

const ResourceHandle& MaterialInstance::GetTexture(U8 textureSlot) const
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

ResourceHandle MaterialInstance::GetTexture(U8 textureSlot)
{
	AssertExpr(textureSlot < TEXTURE_SLOT_COUNT);
	return m_textureSlots[textureSlot];
}

void MaterialInstance::SetShaderTechnique(const ResourceHandle& shaderTechnique)
{
	m_shaderTechnique = shaderTechnique;
}

void MaterialInstance::CommitPropertyChanges()
{
	Rendering::Renderer::UploadDataToBuffer(m_paramBuffer, &m_properties);
}

Rendering::ConstantBufferHandle MaterialInstance::GetParamBuffer() const
{
	return m_paramBuffer;
}
