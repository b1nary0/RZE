#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Assimp/Importer.hpp>
#include <Assimp/config.h>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Math.h>
#include <Utils/PrimitiveDefs.h>

#include <Utils/Platform/File.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace
{
	// #TODO
	// Move these into a common file. They will be used across different classes
	constexpr char kMaterialAssetSuffix[] = { ".materialasset" };

	// A model whose largest extent falls outside this range was probably authored in another unit
	constexpr float k_minPlausibleExtentMetres = 0.01f;
	constexpr float k_maxPlausibleExtentMetres = 500.0f;

	std::string StripAssetNameFromFilePath(const Filepath& filePath)
	{
		const std::string& assetPath = filePath.GetRelativePath();

		size_t pos = assetPath.find_last_of('\\');
		std::string assetName = assetPath.substr(pos + 1, assetPath.size());
		assetName = assetName.substr(0, assetName.find_last_of('.'));

		return assetName;
	}

	Matrix4x4 ToMatrix4x4(aiMatrix4x4 transform)
	{
		// Assimp stores a matrix row by row and the engine's math column by column; both multiply column vectors
		transform.Transpose();
		return Matrix4x4::FromValuePtr(&transform.a1);
	}

	// Source names (FBX especially) can hold namespaces (':'), '#', slashes and spaces; none belong in a file name
	std::string SanitizeFileName(const std::string& name)
	{
		std::string sanitized = name;
		for (char& character : sanitized)
		{
			const bool isControl = static_cast<unsigned char>(character) < 32;
			if (isControl || std::isspace(static_cast<unsigned char>(character)) || std::strchr("\\/:*?\"<>|#", character) != nullptr)
			{
				character = '_';
			}
		}
		return sanitized;
	}
}

aiMatrix4x4 AssimpSourceImporter::GetYUpCorrection(const aiScene& scene)
{
	// FBX's defaults are already the engine's: X right, Y up, Z front
	int32_t upAxis = 1;
	int32_t upAxisSign = 1;
	int32_t frontAxis = 2;
	int32_t frontAxisSign = 1;
	int32_t coordAxis = 0;
	int32_t coordAxisSign = 1;

	if (const aiMetadata* const metadata = scene.mMetaData)
	{
		metadata->Get("UpAxis", upAxis);
		metadata->Get("UpAxisSign", upAxisSign);
		metadata->Get("FrontAxis", frontAxis);
		metadata->Get("FrontAxisSign", frontAxisSign);
		metadata->Get("CoordAxis", coordAxis);
		metadata->Get("CoordAxisSign", coordAxisSign);
	}

	const bool isValid = upAxis >= 0 && upAxis < 3 && frontAxis >= 0 && frontAxis < 3 && coordAxis >= 0 && coordAxis < 3
		&& upAxis != frontAxis && upAxis != coordAxis && frontAxis != coordAxis;
	if (!isValid)
	{
		RZE_LOG_ARGS("AssimpSourceImporter : WARNING axes (up %d, front %d, coord %d) don't make sense; imported as Y-up", upAxis, frontAxis, coordAxis);
		return aiMatrix4x4();
	}

	// Each row picks the source axis that becomes the engine's X (right), Y (up) and Z (front)
	aiVector3D right;
	aiVector3D up;
	aiVector3D front;
	right[coordAxis] = static_cast<ai_real>(coordAxisSign);
	up[upAxis] = static_cast<ai_real>(upAxisSign);
	front[frontAxis] = static_cast<ai_real>(frontAxisSign);

	const aiMatrix4x4 correction(
		right.x, right.y, right.z, 0.0f,
		up.x, up.y, up.z, 0.0f,
		front.x, front.y, front.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	if (!correction.IsIdentity())
	{
		RZE_LOG_ARGS("AssimpSourceImporter : Source declares up axis %d (sign %d); turning it Y-up", upAxis, upAxisSign);
	}
	if (correction.Determinant() < 0.0f)
	{
		RZE_LOG_ARGS("AssimpSourceImporter : WARNING the source's axes are left-handed; it's mirrored into the engine's");
	}

	return correction;
}

std::string AssimpSourceImporter::ResolveTexturePath(const aiScene& scene, const aiMaterial& material, int textureType)
{
	const aiTextureType type = static_cast<aiTextureType>(textureType);
	if (material.GetTextureCount(type) == 0)
	{
		return std::string();
	}

	aiString assimpPath;
	material.GetTexture(type, 0, &assimpPath);
	return ResolveTextureFile(scene, assimpPath.C_Str());
}

std::string AssimpSourceImporter::FindCutoutMaskFallback(const aiScene& scene, const aiMaterial& material)
{
	// #TODO(Transparency) STOPGAP - remove once materials carry real transparency (see Docs/Plans/Transparency.md,
	// Part 2 "Blending").
	//
	// Why this exists: 3ds Max Physical Materials (New Sponza's dirt decals, for one) connect their mask through
	// "3dsMax|Parameters|cutout_map" / "transparency_map". Assimp doesn't map either to aiTextureType_OPACITY; it only
	// keeps them as raw FBX properties ("$raw.<property>|file", semantic aiTextureType_UNKNOWN). Without this the
	// decals draw as solid slabs over the walls.
	//
	// What it gets wrong, on purpose, for now:
	//  - transparency_map is a *blend* amount, not a cutout. We treat it as a cutout because the engine can only
	//    clip (Lighting.hlsli: clip(mask - AlphaCutoff)); soft masks get hard edges at the cutoff.
	//  - The material's scalar transparency ($raw.3dsMax|Parameters|transparency, $mat.transparencyfactor) is ignored.
	//  - Only 3ds Max's property names are known. Maya (Stingray/StandardSurface), Blender and others name theirs
	//    differently and still fall through to opaque.
	//  - The mask's convention is assumed to match ours (white = solid). Max can invert maps in its own graph,
	//    which we can't see here.
	// The proper fix reads these into a real transparency model (alpha blend vs. cutout, a factor, an inverted
	// flag) instead of folding everything into the cutout slot.
	static constexpr const char* k_rawMaskKeys[] =
	{
		"$raw.3dsMax|Parameters|cutout_map|file",
		"$raw.3dsMax|Parameters|transparency_map|file",
	};

	for (const char* rawKey : k_rawMaskKeys)
	{
		aiString rawPath;
		if (material.Get(rawKey, aiTextureType_UNKNOWN, 0, rawPath) == AI_SUCCESS && rawPath.length > 0)
		{
			RZE_LOG_ARGS("AssimpSourceImporter : using %s [%s] as a cutout mask (stopgap; see #TODO(Transparency))", rawKey, rawPath.C_Str());
			return ResolveTextureFile(scene, rawPath.C_Str());
		}
	}

	return std::string();
}

std::string AssimpSourceImporter::ResolveTextureFile(const aiScene& scene, const std::string& sourcePath)
{
	// #TODO(Josh::We need to bake the texture folder structure here, or get on proper asset referencing system)
	const std::string sourceDirectory = m_filepath.GetRelativeDirectoryPath();

	// Embedded ("*0", or by name in newer exporters): written out next to the source, since textures load by path
	const std::pair<const aiTexture*, int> embedded = scene.GetEmbeddedTextureAndIndex(sourcePath.c_str());
	if (embedded.first != nullptr)
	{
		const aiTexture& texture = *embedded.first;
		if (texture.mHeight != 0)
		{
			// Uncompressed texels would need encoding into an image file first
			RZE_LOG_ARGS("AssimpSourceImporter : WARNING embedded texture %d [%s] isn't compressed; skipping it", embedded.second, sourcePath.c_str());
			return std::string();
		}

		const std::string extension = texture.achFormatHint[0] != '\0' ? texture.achFormatHint : "bin";
		const Filepath outputPath(sourceDirectory + m_assetName + "_embedded_" + std::to_string(embedded.second) + "." + extension);

		File outputFile(outputPath);
		outputFile.Open(static_cast<File::EFileOpenMode::Value>(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));
		const Byte* const bytes = reinterpret_cast<const Byte*>(texture.pcData);
		for (U32 byteIndex = 0; byteIndex < texture.mWidth; ++byteIndex)
		{
			outputFile << bytes[byteIndex];
		}
		outputFile.Close();

		RZE_LOG_ARGS("AssimpSourceImporter : %s written from embedded texture %d.", outputPath.GetRelativePath().c_str(), embedded.second);
		return outputPath.GetRelativePath();
	}

	// FBX often keeps the path on the author's machine. Try it next to the source as given (unless it's absolute),
	// then just its file name there, then in a textures folder.
	const std::filesystem::path sourcePathFs(sourcePath);
	const std::string fileName = sourcePathFs.filename().string();

	std::vector<std::string> candidates;
	if (!sourcePathFs.is_absolute() && !sourcePathFs.has_root_name())
	{
		candidates.push_back(sourceDirectory + sourcePath);
	}
	candidates.push_back(sourceDirectory + fileName);
	candidates.push_back(sourceDirectory + "textures\\" + fileName);

	for (const std::string& candidate : candidates)
	{
		const Filepath candidatePath(candidate);
		if (candidatePath.Exists())
		{
			return candidatePath.GetRelativePath();
		}
	}

	RZE_LOG_ARGS("AssimpSourceImporter : WARNING texture [%s] not found next to [%s]; leaving it out", sourcePath.c_str(), m_filepath.GetRelativePath().c_str());
	return std::string();
}

AssimpSourceImporter::AssimpSourceImporter()
{
}

bool AssimpSourceImporter::Import(const Filepath& filePath)
{
	RZE_LOG_ARGS("AssimpSourceImporter : Importing %s...", filePath.GetRelativePath().c_str());

	m_filepath = filePath;
	m_assetName = StripAssetNameFromFilePath(filePath);

	Assimp::Importer ModelImporter;

	const std::string extension = std::filesystem::path(filePath.GetAbsolutePath()).extension().string();
	if (!ModelImporter.IsExtensionSupported(extension))
	{
		RZE_LOG_ARGS("AssimpSourceImporter : This Assimp build can't read %s files [%s].", extension.c_str(), filePath.GetRelativePath().c_str());
		return false;
	}

	// FBX pivots become extra nodes otherwise; folded into the node transforms they bake into the placements the same
	ModelImporter.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* AssimpScene = ModelImporter.ReadFile(filePath.GetAbsolutePath(),
		aiProcessPreset_TargetRealtime_Fast |
		// Source formats are right-handed like the engine, so geometry and winding stay as authored
		// (counter-clockwise front faces). Only V flips, for D3D's top-left texture origin.
		aiProcess_FlipUVs |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph |
		// Converts formats that declare their unit (FBX) to metres. Unitless formats (OBJ) are taken as metres.
		aiProcess_GlobalScale);

	bool bAssimpNotLoaded =
		!AssimpScene
		|| !AssimpScene->mRootNode
		|| (AssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0;

	if (bAssimpNotLoaded)
	{
		// #TODO More informative error message.
		RZE_LOG_ARGS("Failed to load model from [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	// Each mesh is read once, however many nodes place it
	m_meshes.reserve(AssimpScene->mNumMeshes);
	for (U32 meshIndex = 0; meshIndex < AssimpScene->mNumMeshes; ++meshIndex)
	{
		const aiMesh& assimpMesh = *AssimpScene->mMeshes[meshIndex];

		MeshData& meshData = m_meshes.emplace_back();
		meshData.MeshName = assimpMesh.mName.C_Str();

		RZE_LOG_ARGS("AssimpSourceImporter : Processing Mesh %u of %u - %s", meshIndex + 1, AssimpScene->mNumMeshes, meshData.MeshName.c_str());
		ProcessMesh(assimpMesh, *AssimpScene, meshData);
	}

	ProcessNode(*AssimpScene->mRootNode, GetYUpCorrection(*AssimpScene));
	RemoveUnplacedGeometry();

	if (m_placements.empty())
	{
		RZE_LOG_ARGS("No meshes placed in [%s].", filePath.GetRelativePath().c_str());
		return false;
	}

	RZE_LOG_ARGS("AssimpSourceImporter : %u geometries, %u placements (%u mirrored copies)",
		static_cast<U32>(m_meshes.size()), static_cast<U32>(m_placements.size()), static_cast<U32>(m_mirroredGeometry.size()));

	{
		std::vector<BoundingBox> geometryBounds(m_meshes.size());
		for (size_t meshIndex = 0; meshIndex < m_meshes.size(); ++meshIndex)
		{
			for (const MeshVertex& vertex : m_meshes[meshIndex].VertexDataArray)
			{
				geometryBounds[meshIndex].Grow(vertex.Position);
			}
		}

		BoundingBox bounds;
		for (const MeshPlacement& placement : m_placements)
		{
			if (!geometryBounds[placement.GeometryIndex].IsEmpty())
			{
				bounds.Grow(geometryBounds[placement.GeometryIndex].Transformed(placement.LocalTransform));
			}
		}

		const Vector3D extents = bounds.IsEmpty() ? Vector3D() : bounds.GetSize();
		const float largestExtent = std::max({ extents.X(), extents.Y(), extents.Z() });
		if (largestExtent < k_minPlausibleExtentMetres || largestExtent > k_maxPlausibleExtentMetres)
		{
			RZE_LOG_ARGS("AssimpSourceImporter : WARNING [%s] is %.2f m across; its source unit may not be metres. Imported at 1 unit = 1 m; scale it in the editor.",
				filePath.GetRelativePath().c_str(), largestExtent);
		}
	}

	if (!WriteMeshAsset())
	{
		RZE_LOG_ARGS("Failed to write meshasset for [%s]. See logs.", filePath.GetRelativePath().c_str());
		return false;
	}

	for (auto& dataPair : m_materialTable)
	{
		if (!WriteMaterialAsset(dataPair.first, dataPair.second))
		{
			RZE_LOG_ARGS("Failed to write materialAsset for [%s]. See logs.", filePath.GetRelativePath().c_str());
			return false;
		}
	}

	RZE_LOG_ARGS("AssimpSourceImporter : Import Success for %s", filePath.GetRelativePath().c_str());
	return true;
}

void AssimpSourceImporter::ProcessNode(const aiNode& node, const aiMatrix4x4& parentTransform)
{
	const aiMatrix4x4 transform = parentTransform * node.mTransformation;
	const bool isMirrored = transform.Determinant() < 0.0f;

	for (U32 nodeMeshIndex = 0; nodeMeshIndex < node.mNumMeshes; ++nodeMeshIndex)
	{
		const U32 geometryIndex = node.mMeshes[nodeMeshIndex];

		// Unnamed meshes take the name of the first node that places them
		if (m_meshes[geometryIndex].MeshName.empty())
		{
			m_meshes[geometryIndex].MeshName = node.mName.C_Str();
		}

		MeshPlacement& placement = m_placements.emplace_back();
		placement.GeometryIndex = isMirrored ? GetMirroredGeometryIndex(geometryIndex) : geometryIndex;
		placement.LocalTransform = ToMatrix4x4(transform);
	}

	for (U32 childNodeIdx = 0; childNodeIdx < node.mNumChildren; childNodeIdx++)
	{
		ProcessNode(*node.mChildren[childNodeIdx], transform);
	}
}

U32 AssimpSourceImporter::GetMirroredGeometryIndex(U32 geometryIndex)
{
	auto found = m_mirroredGeometry.find(geometryIndex);
	if (found != m_mirroredGeometry.end())
	{
		return found->second;
	}

	// Copied rather than referenced: emplace_back may reallocate m_meshes
	MeshData mirrored = m_meshes[geometryIndex];
	mirrored.MeshName += "_Mirrored";
	// Reversing the whole list reverses each triangle's winding (and the triangles' order, which doesn't matter)
	std::reverse(mirrored.IndexArray.begin(), mirrored.IndexArray.end());

	const U32 mirroredIndex = static_cast<U32>(m_meshes.size());
	m_meshes.push_back(std::move(mirrored));
	m_mirroredGeometry[geometryIndex] = mirroredIndex;

	RZE_LOG_ARGS("AssimpSourceImporter : %s is placed mirrored; drawing it from a copy with reversed winding", m_meshes[geometryIndex].MeshName.c_str());
	return mirroredIndex;
}

void AssimpSourceImporter::RemoveUnplacedGeometry()
{
	std::vector<bool> isPlaced(m_meshes.size(), false);
	for (const MeshPlacement& placement : m_placements)
	{
		isPlaced[placement.GeometryIndex] = true;
	}

	if (std::find(isPlaced.begin(), isPlaced.end(), false) == isPlaced.end())
	{
		return;
	}

	std::vector<U32> newIndex(m_meshes.size(), 0);
	std::vector<MeshData> placedMeshes;
	placedMeshes.reserve(m_meshes.size());
	for (size_t meshIndex = 0; meshIndex < m_meshes.size(); ++meshIndex)
	{
		if (!isPlaced[meshIndex])
		{
			RZE_LOG_ARGS("AssimpSourceImporter : %s isn't placed by any node; skipping it", m_meshes[meshIndex].MeshName.c_str());
			continue;
		}

		newIndex[meshIndex] = static_cast<U32>(placedMeshes.size());
		placedMeshes.push_back(std::move(m_meshes[meshIndex]));
	}

	m_meshes = std::move(placedMeshes);
	for (MeshPlacement& placement : m_placements)
	{
		placement.GeometryIndex = newIndex[placement.GeometryIndex];
	}
}

void AssimpSourceImporter::ProcessMesh(const aiMesh& mesh, const aiScene& scene, MeshData& outMeshData)
{
	bool bHasNormals = mesh.HasNormals();
	bool bHasTextureCoords = mesh.HasTextureCoords(0);
	bool bHasTangents = mesh.HasTangentsAndBitangents();

	for (U32 vertexIdx = 0; vertexIdx < mesh.mNumVertices; vertexIdx++)
	{
		const aiVector3D& assimpVert = mesh.mVertices[vertexIdx];

		Vector3D vertPos(assimpVert.x, assimpVert.y, assimpVert.z);

		MeshVertex vertex;
		vertex.Position = vertPos;

		if (bHasNormals)
		{
			const aiVector3D& assimpNormal = mesh.mNormals[vertexIdx];
			vertex.Normal = Vector3D(assimpNormal.x, assimpNormal.y, assimpNormal.z);
		}

		if (bHasTextureCoords)
		{
			const aiVector3D& assimpUV = mesh.mTextureCoords[0][vertexIdx];
			Vector2D vertUV(assimpUV.x, assimpUV.y);
			vertex.UVData = vertUV;
		}

		if (bHasTangents)
		{
			const aiVector3D& assimpTangent = mesh.mTangents[vertexIdx];
			Vector3D vertTangent(assimpTangent.x, assimpTangent.y, assimpTangent.z);
			vertex.Tangent = vertTangent;
		}

		outMeshData.VertexDataArray.emplace_back(vertex);
	}

	for (U32 faceIdx = 0; faceIdx < mesh.mNumFaces; faceIdx++)
	{
		const aiFace& assimpFace = mesh.mFaces[faceIdx];
		for (U32 indexIdx = 0; indexIdx < assimpFace.mNumIndices; indexIdx++)
		{
			outMeshData.IndexArray.push_back(assimpFace.mIndices[indexIdx]);
		}
	}

	if (mesh.mMaterialIndex < scene.mNumMaterials)
	{
		// Meshes sharing a material share its asset
		auto knownMaterial = m_materialPaths.find(mesh.mMaterialIndex);
		if (knownMaterial != m_materialPaths.end())
		{
			outMeshData.MaterialPath = knownMaterial->second;
			return;
		}

		// #TODO
		// This is just temp
		const int kTextureTypeCount = 5;

		MaterialData materialData;
		materialData.TexturePaths.resize(kTextureTypeCount);

		const aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];
		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);

		materialData.MaterialName = SanitizeFileName(matName.C_Str());
		if (materialData.MaterialName.empty())
		{
			materialData.MaterialName = "Material_" + std::to_string(mesh.mMaterialIndex);
		}

		float shininess = 0.0f;
		if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
		{
			// Source files store a Phong exponent; the shaders use Blinn-Phong, whose half-vector
			// lobe needs ~4x the exponent for a highlight of the same size
			materialData.Properties.Shininess = shininess * 4.0f;
		}

		float opacity = 0.0f;
		if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
		{
			materialData.Properties.Opacity = opacity;
		}

		aiColor3D diffColour;
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColour) == AI_SUCCESS)
		{
		}

		// #TODO
		// Not entirely sure what would come in with > 0 of these texture types (though obviously its valid)
		// so in the interest of being explicit of what RZE supports at the moment, only take the 0th texture data
		// glTF and other PBR sources fill BASE_COLOR and NORMAL_CAMERA instead of DIFFUSE and NORMALS
		const aiTextureType albedoType = mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 ? aiTextureType_DIFFUSE : aiTextureType_BASE_COLOR;
		const aiTextureType normalType = mat->GetTextureCount(aiTextureType_NORMALS) > 0 ? aiTextureType_NORMALS : aiTextureType_NORMAL_CAMERA;

		// Each slot only counts as used once its texture is found
		const auto setTexture = [this, &scene, mat, &materialData](aiTextureType type, size_t slot, U8 flag)
		{
			materialData.TexturePaths[slot] = ResolveTexturePath(scene, *mat, type);
			if (!materialData.TexturePaths[slot].empty())
			{
				materialData.TextureFlags |= flag;
			}
		};

		setTexture(albedoType, 0, MaterialData::ETextureFlags::TEXTUREFLAG_ALBEDO);
		setTexture(aiTextureType_SPECULAR, 1, MaterialData::ETextureFlags::TEXTUREFLAG_SPECULAR);
		setTexture(normalType, 2, MaterialData::ETextureFlags::TEXTUREFLAG_NORMAL);

		// Cutout mask (map_d in .mtl)
		setTexture(aiTextureType_OPACITY, 3, MaterialData::ETextureFlags::TEXTUREFLAG_OPACITY);

		// #TODO(Transparency) Stopgap: masks Assimp leaves unmapped (3ds Max cutout/transparency maps) become cutouts.
		// Remove when the material format has real transparency; FindCutoutMaskFallback lists what this gets wrong.
		if (materialData.TexturePaths[3].empty())
		{
			materialData.TexturePaths[3] = FindCutoutMaskFallback(scene, *mat);
			if (!materialData.TexturePaths[3].empty())
			{
				materialData.TextureFlags |= MaterialData::ETextureFlags::TEXTUREFLAG_OPACITY;
			}
		}

		// #TODO(Transparency) (Docs/Plans/Transparency.md, Part 1) Glass comes in opaque. Only AI_MATKEY_OPACITY is read (into Properties.Opacity, above),
		// and while it reaches the shaders' material buffer (PixelResources.hlsli) no shader uses it and nothing
		// draws blended. FBX's $mat.transparencyfactor and 3ds Max's $raw.3dsMax|Parameters|transparency aren't read.

		// map_d pointing at the diffuse texture means "use the diffuse alpha"; a dedicated mask stores it in its colour channels
		if (!materialData.TexturePaths[3].empty() && materialData.TexturePaths[3] == materialData.TexturePaths[0])
		{
			materialData.TextureFlags |= MaterialData::ETextureFlags::TEXTUREFLAG_OPACITY_IN_ALPHA;
		}

		// Grayscale height/bump map (map_bump in .mtl). Distinct from tangent-space normal maps (map_Kn).
		setTexture(aiTextureType_HEIGHT, 4, MaterialData::ETextureFlags::TEXTUREFLAG_HEIGHT);

		// #TODO really gross string manip lol
		const std::string materialFolder = "ProjectData/Material/" + m_assetName + "/";
		std::string assetFilepathRelative = materialFolder + materialData.MaterialName + kMaterialAssetSuffix;
		// Another material already has this name: keep both by telling this one apart by its index
		if (m_materialTable.count(assetFilepathRelative))
		{
			assetFilepathRelative = materialFolder + materialData.MaterialName + "_" + std::to_string(mesh.mMaterialIndex) + kMaterialAssetSuffix;
		}

		outMeshData.MaterialPath = assetFilepathRelative;
		m_materialPaths[mesh.mMaterialIndex] = assetFilepathRelative;
		m_materialTable[assetFilepathRelative] = std::move(materialData);
	}
}

bool AssimpSourceImporter::WriteMeshAsset()
{
	MeshAssetWriter assetWriter;
	assetWriter.SetAssetName(m_assetName);
	assetWriter.SetMeshData(std::move(m_meshes));
	assetWriter.SetPlacements(std::move(m_placements));

	assetWriter.Write();

	return true;
}

bool AssimpSourceImporter::WriteMaterialAsset(const std::string& relativePath, const MaterialData& materialData)
{
	AssertExpr(!relativePath.empty());

	Filepath outputPath(relativePath);
	if (!outputPath.Exists())
	{
		Filepath::CreateDir(outputPath.GetAbsoluteDirectoryPath());
	}

	// #TODO json files are too large. need to compress and write the stream - see zlib
	File outputFile(outputPath);
	outputFile.Open((File::EFileOpenMode::Value)(File::EFileOpenMode::Write | File::EFileOpenMode::Binary));

	// sizeof(size_t) * 3 == bufSize + nameSizeBytes + textureCount
	size_t bufSize = (sizeof(size_t) * 3) + materialData.MaterialName.length() + sizeof(MaterialData::MaterialProperties) + sizeof(U8);
	for (const std::string& texturePath : materialData.TexturePaths)
	{
		// count the bytes of the texture string size in sizeof(size_t) here
		bufSize += sizeof(size_t) + texturePath.size();
	}

	ByteStream byteStream(outputPath.GetRelativePath(), bufSize);

	byteStream.WriteBytes(&bufSize, sizeof(size_t));

	const size_t nameSizeBytes = materialData.MaterialName.size();
	byteStream.WriteBytes(&nameSizeBytes, sizeof(size_t));
	byteStream.WriteBytes(materialData.MaterialName.data(), nameSizeBytes);

	byteStream.WriteBytes(&materialData.Properties, sizeof(MaterialData::MaterialProperties));
	byteStream.WriteBytes(&materialData.TextureFlags, sizeof(U8));

	const size_t kTextureCount = materialData.TexturePaths.size();
	byteStream.WriteBytes(&kTextureCount, sizeof(size_t));
	for (int textureIdx = 0; textureIdx < kTextureCount; ++textureIdx)
	{
		const std::string& texturePath = materialData.TexturePaths[textureIdx];

		const size_t pathSizeBytes = texturePath.size();
		byteStream.WriteBytes(&pathSizeBytes, sizeof(size_t));
		byteStream.WriteBytes(texturePath.data(), pathSizeBytes);
	}

	AssertExpr(byteStream.GetNumBytesWritten() == bufSize);

	const Byte* bytes = byteStream.GetBytes();
	for (size_t byteIndex = 0; byteIndex < byteStream.GetNumBytesWritten(); ++byteIndex)
	{
		outputFile << bytes[byteIndex];
	}

	outputFile.Close();

	RZE_LOG_ARGS("AssimpSourceImporter : %s written.", outputPath.GetRelativePath().c_str());

	return true;
}

bool AssimpSourceImporter::WriteTextureAsset()
{
	return false;
}
