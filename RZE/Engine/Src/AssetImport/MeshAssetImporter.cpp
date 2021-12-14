#include <StdAfx.h>

#include <AssetImport/MeshAssetImporter.h>

namespace
{
	constexpr char kAssetImportVersionKey[] = { "meshasset_version" };
	constexpr int kAssetImportVersion = 1;

	constexpr char kAssetStartKey[] = { "asset_start" };

	constexpr char kVertexDataSizeKey[] = { "vertex_data_size" };
	constexpr char kVertexDataKey[] = { "vertex_data" };

	constexpr char kIndexDataSizeKey[] = { "index_data_size" };
	constexpr char kIndexDataKey[] = { "index_data" };
}

bool MeshAssetImporter::Import(const FilePath& filePath)
{
	File meshFile(filePath);
	AssertExpr(meshFile.IsValid());
	meshFile.Close();

	if (meshFile.Empty())
	{
		LOG_CONSOLE_ARGS("Error reading file [%s]", filePath.GetRelativePath().c_str());
		return false;
	}

	rapidjson::Document meshDoc;
	meshDoc.Parse(meshFile.Content().c_str());

	rapidjson::Value::MemberIterator root = meshDoc.FindMember(kAssetStartKey);

	if (root == meshDoc.MemberEnd())
	{
		// #TODO log a helpful message
		return false;
	}

	rapidjson::Value& rootVal = root->value;
	for (auto& member = rootVal.MemberBegin(); member != rootVal.MemberEnd(); ++member)
	{
		std::string memberName = member->name.GetString();
		if (memberName == kAssetImportVersionKey)
		{
			const int fileVersion = member->value.GetInt();
			AssertExpr(fileVersion == kAssetImportVersion);
		}
		else
		{
			// Everything else should be a mesh definition, nothing else should be on this
			// layer except meshasset_version.
			rapidjson::Value& meshMemberVal = member->value;
			
			rapidjson::Value::MemberIterator vertexDataSizeMember = meshMemberVal.FindMember(kVertexDataSizeKey);
			if (vertexDataSizeMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), kVertexDataSizeKey);
				return false;
			}

			rapidjson::Value::MemberIterator vertexDataMember = meshMemberVal.FindMember(kVertexDataKey);
			if (vertexDataMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), kVertexDataKey);
				return false;
			}

			rapidjson::Value::MemberIterator indexDataSizeMember = meshMemberVal.FindMember(kIndexDataSizeKey);
			if (indexDataSizeMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), kIndexDataSizeKey);
				return false;
			}

			rapidjson::Value::MemberIterator indexDataMember = meshMemberVal.FindMember(kIndexDataKey);
			if (indexDataMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), kIndexDataKey);
				return false;
			}

			const int vertexCount = vertexDataSizeMember->value.GetInt();
			const int vertexStride = sizeof(MeshVertex) / sizeof(float);
			const int vertexDataSize = vertexCount * vertexStride;
			const int indexDataSize = indexDataSizeMember->value.GetInt();

			std::vector<float> floatArray;
			floatArray.reserve(vertexDataSize);
			for (int i = 0; i < vertexDataSize; ++i)
			{
				floatArray.push_back(static_cast<float>(vertexDataMember->value[i].GetDouble()));
			}

			MeshGeometry meshGeometry(vertexCount, indexDataSize);
			for (int i = 0; i < vertexCount; ++i)
			{
				MeshVertex* vertLikeTonyHawk = static_cast<MeshVertex*>((void*)&floatArray[i * vertexStride]);
				meshGeometry.AddVertex(*vertLikeTonyHawk);
			}

			for (int i = 0; i < indexDataSize; ++i)
			{
				meshGeometry.AddIndex(indexDataMember->value[i].GetUint());
			}

			meshGeometry.AllocateData();
			mMeshGeometry.emplace_back(std::move(meshGeometry));
		}
	}

	return true;
}
