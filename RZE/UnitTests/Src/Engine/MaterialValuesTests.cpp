#include <doctest/doctest.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialValues.h>

// Only the pure parts: SetValues itself needs the renderer and resources

namespace
{
	using MaterialData = MaterialAssetImporter::MaterialData;
	using EBlendMode = MaterialValues::EBlendMode;

	MaterialValues WithTextures(std::initializer_list<MaterialInstance::TextureSlot> slots)
	{
		MaterialValues values;
		for (MaterialInstance::TextureSlot slot : slots)
		{
			values.Textures[slot] = Filepath("Assets/Textures/Test.png");
		}
		return values;
	}
}

TEST_CASE("A material asset's values carry its blend mode, factors, textures and mask flags")
{
	MaterialData materialData;
	materialData.BlendMode = EBlendMode::Transparent;
	materialData.Properties.Opacity = 0.61f;
	materialData.Properties.Shininess = 32.0f;
	materialData.TexturePaths = { "Assets/Textures/Diffuse.png", "", "", "Assets/Textures/Mask.png", "" };
	materialData.TextureFlags = MaterialData::TEXTUREFLAG_ALBEDO | MaterialData::TEXTUREFLAG_OPACITY | MaterialData::TEXTUREFLAG_INVERT_OPACITY;

	const MaterialValues values = MaterialValues::FromMaterialData(materialData);
	CHECK(values.BlendMode == EBlendMode::Transparent);
	CHECK(values.Opacity == doctest::Approx(0.61f));
	CHECK(values.Shininess == doctest::Approx(32.0f));
	CHECK(values.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE].GetRelativePath() == Filepath("Assets/Textures/Diffuse.png").GetRelativePath());
	CHECK(values.Textures[MaterialInstance::TEXTURE_SLOT_OPACITY].IsValid());
	// Empty paths stay empty slots
	CHECK_FALSE(values.Textures[MaterialInstance::TEXTURE_SLOT_SPECULAR].IsValid());
	CHECK_FALSE(values.Textures[MaterialInstance::TEXTURE_SLOT_HEIGHT].IsValid());
	CHECK(values.InvertOpacityMask);
	CHECK_FALSE(values.OpacityMaskInAlpha);
}

TEST_CASE("A material asset's tint is its values' tint")
{
	MaterialData materialData;
	materialData.Tint = Vector3D(0.2f, 0.4f, 0.6f);
	const MaterialValues values = MaterialValues::FromMaterialData(materialData);
	CHECK(values.Tint.X() == doctest::Approx(0.2f));
	CHECK(values.Tint.Y() == doctest::Approx(0.4f));
	CHECK(values.Tint.Z() == doctest::Approx(0.6f));
}

TEST_CASE("A material asset with fewer texture paths than slots leaves the rest empty")
{
	MaterialData materialData;
	materialData.TexturePaths = { "Assets/Textures/Diffuse.png" };

	const MaterialValues values = MaterialValues::FromMaterialData(materialData);
	CHECK(values.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE].IsValid());
	for (size_t slot = 1; slot < MaterialValues::k_textureSlotCount; ++slot)
	{
		CHECK_FALSE(values.Textures[slot].IsValid());
	}
}

TEST_CASE("Only a blended material draws with its opacity; the rest are solid on the GPU")
{
	MaterialValues values;
	values.Opacity = 0.4f;

	values.BlendMode = EBlendMode::Opaque;
	CHECK(MaterialInstance::BuildParams(values).Opacity == doctest::Approx(1.0f));
	CHECK(MaterialInstance::BuildParams(values).OpacityMaskIsCoverage == doctest::Approx(0.0f));

	values.BlendMode = EBlendMode::Cutout;
	CHECK(MaterialInstance::BuildParams(values).Opacity == doctest::Approx(1.0f));
	CHECK(MaterialInstance::BuildParams(values).OpacityMaskIsCoverage == doctest::Approx(0.0f));

	for (EBlendMode blended : { EBlendMode::Transparent, EBlendMode::Premultiplied, EBlendMode::Additive, EBlendMode::Multiply })
	{
		values.BlendMode = blended;
		CHECK(MaterialInstance::BuildParams(values).Opacity == doctest::Approx(0.4f));
		// A blended material's mask fades rather than cuts
		CHECK(MaterialInstance::BuildParams(values).OpacityMaskIsCoverage == doctest::Approx(1.0f));
	}
}

TEST_CASE("A material's GPU parameters say which mask and height textures it has, and how to read the mask")
{
	MaterialValues values = WithTextures({ MaterialInstance::TEXTURE_SLOT_OPACITY });
	values.Shininess = 12.0f;
	values.InvertOpacityMask = true;

	MaterialInstance::MaterialParams params = MaterialInstance::BuildParams(values);
	CHECK(params.Shininess == doctest::Approx(12.0f));
	CHECK(params.HasOpacityMask == doctest::Approx(1.0f));
	CHECK(params.HasHeightMap == doctest::Approx(0.0f));
	CHECK(params.InvertOpacityMask == doctest::Approx(1.0f));
	CHECK(params.OpacityMaskInAlpha == doctest::Approx(0.0f));

	values = WithTextures({ MaterialInstance::TEXTURE_SLOT_HEIGHT });
	values.OpacityMaskInAlpha = true;
	params = MaterialInstance::BuildParams(values);
	CHECK(params.HasOpacityMask == doctest::Approx(0.0f));
	CHECK(params.HasHeightMap == doctest::Approx(1.0f));
	CHECK(params.OpacityMaskInAlpha == doctest::Approx(1.0f));
}

TEST_CASE("A material's GPU parameters carry its tint, and whether a diffuse texture is there for it to tint")
{
	MaterialValues values = WithTextures({ MaterialInstance::TEXTURE_SLOT_DIFFUSE });
	values.Tint = Vector3D(0.9f, 0.5f, 0.1f);
	MaterialInstance::MaterialParams params = MaterialInstance::BuildParams(values);
	CHECK(params.HasDiffuseMap == doctest::Approx(1.0f));
	CHECK(params.Tint[0] == doctest::Approx(0.9f));
	CHECK(params.Tint[1] == doctest::Approx(0.5f));
	CHECK(params.Tint[2] == doctest::Approx(0.1f));

	// A normal map alone: the tint is the colour, rather than whatever diffuse texture was bound last
	params = MaterialInstance::BuildParams(WithTextures({ MaterialInstance::TEXTURE_SLOT_NORMAL }));
	CHECK(params.HasDiffuseMap == doctest::Approx(0.0f));

	// A material made in code binds its own textures, and keeps these
	CHECK(MaterialInstance::MaterialParams().HasDiffuseMap == doctest::Approx(1.0f));
	CHECK(MaterialInstance::MaterialParams().Tint[0] == doctest::Approx(1.0f));
}

TEST_CASE("A material's shader follows its textures")
{
	using EShader = MaterialInstance::EShader;

	CHECK(MaterialInstance::ChooseShader(MaterialValues()) == EShader::Untextured);
	CHECK(MaterialInstance::ChooseShader(WithTextures({ MaterialInstance::TEXTURE_SLOT_DIFFUSE })) == EShader::DiffuseOnly);
	// Any texture short of all three of diffuse, specular and normal
	CHECK(MaterialInstance::ChooseShader(WithTextures({ MaterialInstance::TEXTURE_SLOT_OPACITY })) == EShader::DiffuseOnly);
	CHECK(MaterialInstance::ChooseShader(WithTextures({ MaterialInstance::TEXTURE_SLOT_DIFFUSE, MaterialInstance::TEXTURE_SLOT_NORMAL })) == EShader::DiffuseOnly);
	CHECK(MaterialInstance::ChooseShader(WithTextures({ MaterialInstance::TEXTURE_SLOT_DIFFUSE, MaterialInstance::TEXTURE_SLOT_SPECULAR, MaterialInstance::TEXTURE_SLOT_NORMAL })) == EShader::Full);
	CHECK(MaterialInstance::ChooseShader(WithTextures({ MaterialInstance::TEXTURE_SLOT_DIFFUSE, MaterialInstance::TEXTURE_SLOT_SPECULAR, MaterialInstance::TEXTURE_SLOT_NORMAL, MaterialInstance::TEXTURE_SLOT_HEIGHT })) == EShader::Full);
}
