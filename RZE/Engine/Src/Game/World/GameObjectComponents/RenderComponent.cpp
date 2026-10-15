#include <StdAfx.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Resources/ResourceDependencyList.h>

#include <Game/GameScene.h>
#include <Game/StaticMeshResource.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialResource.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/RenderScene.h>
#include <Graphics/Shader.h>

#include <Utils/DebugUtils/Debug.h>

#include "Graphics/Texture2D.h"

namespace
{
	// Distinct so both are readable when drawn together
	const Vector3D k_meshBoundsColour(1.0f, 0.85f, 0.0f);
	const Vector3D k_subMeshBoundsColour(0.0f, 0.8f, 1.0f);

	constexpr char k_textureFilter[] = "*.png;*.jpg;*.jpeg;*.tga;*.bmp";

	// Changes one of slot's overrides, leaving the rest as they are
	template <typename TChange>
	void ChangeOverrides(MaterialSlot& slot, TChange change)
	{
		MaterialOverrides overrides = slot.GetOverrides();
		change(overrides);
		slot.SetOverrides(overrides);
	}

	// One of a material slot's values as an overridable property: shows what the slot draws with, and setting or
	// resetting it changes the slot's overrides. getValue takes the slot's MaterialValues and returns the value;
	// getOverride takes its MaterialOverrides (const or not) and returns the matching std::optional.
	template <typename TValue, typename TGetValue, typename TGetOverride>
	PropertyDescriptor& DescribeOverridable(PropertyBuilder<MaterialSlot>& slot, const char* name, TGetValue getValue, TGetOverride getOverride)
	{
		return slot.Value<TValue>(name,
			[getValue](const MaterialSlot& materialSlot) { return getValue(materialSlot.GetValues()); },
			[getOverride](MaterialSlot& materialSlot, const TValue& value)
			{
				ChangeOverrides(materialSlot, [&getOverride, &value](MaterialOverrides& overrides) { getOverride(overrides) = value; });
			})
			.Overridable<MaterialSlot>(
				[getOverride](const MaterialSlot& materialSlot) { return getOverride(materialSlot.GetOverrides()).has_value(); },
				[getOverride](MaterialSlot& materialSlot)
				{
					ChangeOverrides(materialSlot, [&getOverride](MaterialOverrides& overrides) { getOverride(overrides).reset(); });
				});
	}

	// The texture a slot draws with in textureSlot, as a preview; empty if it has none
	PropertyTexture GetTexturePreview(const MaterialSlot& slot, U8 textureSlot)
	{
		PropertyTexture preview;
		if (const Texture2D* const texture = slot.GetMaterial().GetTextureResource(textureSlot))
		{
			preview.Handle = texture->GetPlatformObject();
			preview.Size = texture->GetDimensions();
		}
		return preview;
	}

	// The names of every blend mode, in their values' order
	std::vector<std::string> GetBlendModeNames()
	{
		std::vector<std::string> names;
		for (U8 mode = 0;; ++mode)
		{
			const std::string name = MaterialAssetImporter::GetBlendModeName(static_cast<MaterialValues::EBlendMode>(mode));
			if (name == "Unknown")
			{
				return names;
			}
			names.push_back(name);
		}
	}

	// A material slot's properties: each of its values, overridable, with what it draws with
	void DescribeMaterialSlot(PropertyBuilder<MaterialSlot>& slot)
	{
		using EBlendMode = MaterialValues::EBlendMode;

		slot.DisplayName([](const MaterialSlot& materialSlot) { return materialSlot.GetName(); });
		slot.Icon([](const MaterialSlot& materialSlot) { return GetTexturePreview(materialSlot, MaterialInstance::TEXTURE_SLOT_DIFFUSE); });

		// What it draws with first, then its values in sections
		slot.Value<Filepath>("Material",
			[](const MaterialSlot& materialSlot) { return materialSlot.GetMaterialPath(); },
			[](MaterialSlot& materialSlot, const Filepath& path)
			{
				ChangeOverrides(materialSlot, [&path](MaterialOverrides& overrides) { overrides.Material = path; });
			})
			.Overridable<MaterialSlot>(
				[](const MaterialSlot& materialSlot) { return materialSlot.GetOverrides().Material.has_value(); },
				[](MaterialSlot& materialSlot) { ChangeOverrides(materialSlot, [](MaterialOverrides& overrides) { overrides.Material.reset(); }); })
			.Asset("Material", AssetPaths::GetMaterialFolder().GetRelativePath().c_str(), (std::string("*") + AssetPaths::k_materialExtension).c_str())
			.Thumbnail<MaterialSlot>([](const MaterialSlot& materialSlot) { return GetTexturePreview(materialSlot, MaterialInstance::TEXTURE_SLOT_DIFFUSE); })
			.ToolTip("The material asset it draws with; the values below can be overridden on top of it");
		slot.Value<std::string>("Shader", [](const MaterialSlot& materialSlot)
			{
				const PixelShader* const shader = materialSlot.GetMaterial().GetPixelShader();
				return shader != nullptr ? shader->GetName() : std::string("(none)");
			});

		slot.Enum<EBlendMode>("Blend", GetBlendModeNames(),
			[](const MaterialSlot& materialSlot) { return materialSlot.GetValues().BlendMode; },
			[](MaterialSlot& materialSlot, EBlendMode blendMode)
			{
				ChangeOverrides(materialSlot, [blendMode](MaterialOverrides& overrides) { overrides.BlendMode = blendMode; });
			})
			.Overridable<MaterialSlot>(
				[](const MaterialSlot& materialSlot) { return materialSlot.GetOverrides().BlendMode.has_value(); },
				[](MaterialSlot& materialSlot) { ChangeOverrides(materialSlot, [](MaterialOverrides& overrides) { overrides.BlendMode.reset(); }); })
			.ToolTip("How it covers what's behind it")
			.InGroup("Surface");

		DescribeOverridable<float>(slot, "Opacity",
			[](const MaterialValues& values) { return values.Opacity; },
			[](auto& overrides) -> auto& { return overrides.Opacity; })
			.Range(0.0f, 1.0f).As(EPropertyWidget::Slider).DisplayFormat("%.2f")
			.EnabledIf<MaterialSlot>([](const MaterialSlot& materialSlot) { return MaterialAssetImporter::IsBlended(materialSlot.GetValues().BlendMode); },
				"Only blended modes use opacity")
			.InGroup("Surface");

		DescribeOverridable<float>(slot, "Shininess",
			[](const MaterialValues& values) { return values.Shininess; },
			[](auto& overrides) -> auto& { return overrides.Shininess; })
			// Blinn-Phong exponents: imported ones are 4x the source's Phong exponent, so they run into the thousands
			.Range(0.0f, 16384.0f).As(EPropertyWidget::Drag).StepSize(0.5f).DisplayFormat("%.1f")
			.InGroup("Surface");

		for (U8 textureSlot = 0; textureSlot < MaterialInstance::TEXTURE_SLOT_COUNT; ++textureSlot)
		{
			DescribeOverridable<Filepath>(slot, MaterialInstance::GetTextureSlotName(static_cast<MaterialInstance::TextureSlot>(textureSlot)),
				[textureSlot](const MaterialValues& values) { return values.Textures[textureSlot]; },
				[textureSlot](auto& overrides) -> auto& { return overrides.Textures[textureSlot]; })
				.Asset("Texture", "Assets", k_textureFilter)
				.Thumbnail<MaterialSlot>([textureSlot](const MaterialSlot& materialSlot) { return GetTexturePreview(materialSlot, textureSlot); })
				.Clearable()
				.InGroup("Textures");
		}

		const auto hasOpacityTexture = [](const MaterialSlot& materialSlot)
			{
				return materialSlot.GetValues().Textures[MaterialInstance::TEXTURE_SLOT_OPACITY].IsValid();
			};
		DescribeOverridable<bool>(slot, "Invert",
			[](const MaterialValues& values) { return values.InvertOpacityMask; },
			[](auto& overrides) -> auto& { return overrides.InvertOpacityMask; })
			.EnabledIf<MaterialSlot>(hasOpacityTexture, "Needs an opacity texture")
			.ToolTip("The opacity texture's white means see-through")
			.InGroup("Opacity Mask");
		DescribeOverridable<bool>(slot, "From Alpha",
			[](const MaterialValues& values) { return values.OpacityMaskInAlpha; },
			[](auto& overrides) -> auto& { return overrides.OpacityMaskInAlpha; })
			.EnabledIf<MaterialSlot>(hasOpacityTexture, "Needs an opacity texture")
			.ToolTip("The mask is in the opacity texture's alpha rather than its red")
			.InGroup("Opacity Mask");
	}
}

void RenderComponent::DescribeProperties(PropertyBuilder<RenderComponent>& properties)
{
	properties.AssetPath("Mesh", &RenderComponent::GetMeshPath, &RenderComponent::LoadMesh)
		.Asset("RZE MeshAsset", AssetPaths::GetMeshFolder().GetRelativePath().c_str(), (std::string("*") + AssetPaths::k_meshExtension).c_str())
		.Clearable();
	properties.Vector3("Size", &RenderComponent::GetMeshSize)
		.DisplayFormat("%.2f m").ToolTip("Along the object's own axes, including its scale");
	properties.Bool("Mesh Bounds", &RenderComponent::IsDrawingMeshBounds, &RenderComponent::SetDrawMeshBounds).EditorOnly()
		.InGroup("Debug Draw");
	properties.Bool("Sub-Mesh Bounds", &RenderComponent::IsDrawingSubMeshBounds, &RenderComponent::SetDrawSubMeshBounds).EditorOnly()
		.InGroup("Debug Draw");
	// Each of the mesh's material slots, as this object draws it; editing a value overrides it for this object only
	properties.Array<MaterialSlot>("Materials",
		[](const RenderComponent& renderComponent) -> const std::vector<MaterialSlot>&
		{
			static const std::vector<MaterialSlot> s_noSlots;
			return renderComponent.m_meshInstance != nullptr ? renderComponent.m_meshInstance->GetMaterialSlots() : s_noSlots;
		},
		[](RenderComponent& renderComponent) -> std::vector<MaterialSlot>&
		{
			// Never added to: a component without a mesh has no slots to edit
			static std::vector<MaterialSlot> s_noSlots;
			return renderComponent.m_meshInstance != nullptr ? renderComponent.m_meshInstance->GetMaterialSlots() : s_noSlots;
		},
		[](const MaterialSlot& slot) { return slot.GetName(); },
		&DescribeMaterialSlot);
}

const Filepath& RenderComponent::GetMeshPath() const
{
	static const Filepath s_noMesh;
	return m_meshInstance != nullptr ? m_meshInstance->GetResourcePath() : s_noMesh;
}

void RenderComponent::LoadMesh(const Filepath& path)
{
	if (path.GetRelativePath() == GetMeshPath().GetRelativePath())
	{
		return;
	}

	std::shared_ptr<StaticMeshInstance> meshInstance;
	if (path.IsValid())
	{
		const ResourceHandle meshResource = RZE().GetResourceHandler().LoadResource<StaticMeshResource>(path);
		meshInstance = StaticMeshResource::CreateInstance(meshResource);
	}

	SetMeshInstance(meshInstance);
}

void RenderComponent::SetMeshInstance(const std::shared_ptr<StaticMeshInstance>& meshInstance, EMaterialOverrides overrides)
{
	if (overrides == EMaterialOverrides::CarryOver && m_meshInstance != nullptr && meshInstance != nullptr && meshInstance != m_meshInstance)
	{
		meshInstance->SetOverrides(m_meshInstance->GetOverrides());
	}

	m_meshInstance = meshInstance;

	if (m_renderObject != nullptr)
	{
		if (m_meshInstance != nullptr)
		{
			m_renderObject->SetMeshInstance(m_meshInstance);
		}
		else
		{
			m_renderScene->DestroyRenderObject(m_renderObject);
		}
		GetOwner()->MarkHierarchyBoundsChanged();
		return;
	}

	// A component not yet in a scene creates its render object in OnAddToScene
	if (m_owner != nullptr && m_owner->IsInScene())
	{
		CreateRenderObject();
	}
}

Vector3D RenderComponent::GetMeshSize() const
{
	if (m_meshInstance == nullptr || m_owner == nullptr)
	{
		return Vector3D();
	}

	return m_meshInstance->GetMesh().GetBounds().GetSize() * m_owner->GetTransformComponent()->GetScale();
}

void RenderComponent::SetDrawMeshBounds(bool draw)
{
	m_drawMeshBounds = draw;
}

BoundingBox RenderComponent::GetWorldBounds() const
{
	return m_renderObject != nullptr ? m_renderObject->GetBounds() : BoundingBox();
}

BoundingBox RenderComponent::GetLocalBounds() const
{
	return m_renderObject != nullptr ? m_renderObject->GetLocalBounds() : BoundingBox();
}

void RenderComponent::CreateRenderObject()
{
	// Added without a mesh (e.g. via the editor); a render object is created once a mesh is selected
	if (m_meshInstance == nullptr)
	{
		return;
	}

	if (m_renderObject == nullptr)
	{
		// #TODO We should probably make RenderComponent a TransformComponent since a mesh without a transform is useless.
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		m_renderObject = m_renderScene->CreateRenderObject(m_meshInstance);
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());
		// Offset by one because IDs start at 0, which picking uses for "nothing"
		m_renderObject->SetPickID(GetOwner()->GetID() + 1);
		GetOwner()->MarkHierarchyBoundsChanged();

		return;
	}

	RZE_LOG("RenderComponent already has a RenderObject when calling CreateRenderObject()");
}

void RenderComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();
}

void RenderComponent::OnAddToScene()
{
	CreateRenderObject();
}

void RenderComponent::OnRemoveFromScene()
{
	m_renderScene->DestroyRenderObject(m_renderObject);
	GetOwner()->MarkHierarchyBoundsChanged();
}

void RenderComponent::Update()
{
	if (m_renderObject != nullptr)
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		// @TODO Look into ways to avoid this
		m_renderObject->SetTransform(transformComponent->GetWorldMatrix());

		if (m_drawMeshBounds || m_drawSubMeshBounds)
		{
			DrawBounds();
		}
	}
}

void RenderComponent::DrawBounds()
{
	const Matrix4x4& worldMatrix = m_renderObject->GetTransform();

	if (m_drawMeshBounds)
	{
		m_renderScene->DrawWireBox(m_meshInstance->GetMesh().GetBounds(), worldMatrix, k_meshBoundsColour);
	}

	if (m_drawSubMeshBounds)
	{
		for (const MeshDraw& draw : m_renderObject->GetDraws())
		{
			m_renderScene->DrawWireBox(draw.Geometry->GetBounds(), *draw.Transform, k_subMeshBoundsColour);
		}
	}
}

void RenderComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("RenderComponent");
	writer.StartObject();
	{
		writer.Key("ResourcePath");
		writer.String(GetMeshPath().GetRelativePath().c_str());

		// The mesh instance's material overrides: only what's overridden, by slot name
		const MaterialOverridesByName overrides = m_meshInstance != nullptr ? m_meshInstance->GetOverrides() : MaterialOverridesByName();
		if (!overrides.empty())
		{
			writer.Key("MaterialOverrides");
			writer.StartObject();
			for (const auto& [slotName, slotOverrides] : overrides)
			{
				writer.Key(slotName.c_str());
				slotOverrides.Serialize(writer);
			}
			writer.EndObject();
		}
	}
	writer.EndObject();
}

void RenderComponent::GatherResourceDependencies(const rapidjson::Value& componentData, ResourceDependencyList& outDependencies)
{
	rapidjson::Value::ConstMemberIterator resourcePath = componentData.FindMember("ResourcePath");
	if (resourcePath != componentData.MemberEnd() && resourcePath->value.IsString())
	{
		outDependencies.Add<StaticMeshResource>(resourcePath->value.GetString());
	}

	// What its material overrides swap in, so applying them is a cache hit too
	const rapidjson::Value::ConstMemberIterator overridesData = componentData.FindMember("MaterialOverrides");
	if (overridesData != componentData.MemberEnd() && overridesData->value.IsObject())
	{
		for (rapidjson::Value::ConstMemberIterator it = overridesData->value.MemberBegin(); it != overridesData->value.MemberEnd(); ++it)
		{
			const MaterialOverrides overrides = MaterialOverrides::Deserialize(it->value);
			if (overrides.Material.has_value())
			{
				outDependencies.Add<MaterialResource>(overrides.Material->GetRelativePath());
			}
			for (const std::optional<Filepath>& texture : overrides.Textures)
			{
				if (texture.has_value() && texture->IsValid())
				{
					outDependencies.Add<Texture2D>(texture->GetRelativePath());
				}
			}
		}
	}
}

void RenderComponent::Deserialize(const rapidjson::Value& data)
{
	// Preloaded by GatherResourceDependencies during an async scene load, so this is normally a cache hit
	LoadMesh(Filepath(data["ResourcePath"].GetString()));

	// Onto the mesh instance; dropped if the mesh didn't load
	const rapidjson::Value::ConstMemberIterator overridesData = data.FindMember("MaterialOverrides");
	if (m_meshInstance != nullptr && overridesData != data.MemberEnd() && overridesData->value.IsObject())
	{
		MaterialOverridesByName overrides;
		for (rapidjson::Value::ConstMemberIterator it = overridesData->value.MemberBegin(); it != overridesData->value.MemberEnd(); ++it)
		{
			overrides[it->name.GetString()] = MaterialOverrides::Deserialize(it->value);
		}
		m_meshInstance->SetOverrides(overrides);
	}
}

