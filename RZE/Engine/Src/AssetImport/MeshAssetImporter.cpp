#include <StdAfx.h>

#include <AssetImport/MeshAssetImporter.h>

namespace
{
	constexpr char kAssetImportVersionKey[] = { "meshasset_version" };
	constexpr int kAssetImportVersion = 1;
}

bool MeshAssetImporter::Import(const FilePath& filePath)
{
	File meshFile(filePath);
	AssertExpr(meshFile.IsValid());
	meshFile.Close();

	rapidjson::Document meshDoc;
	meshDoc.Parse(meshFile.Content().c_str());

	rapidjson::Value::MemberIterator root = meshDoc.FindMember("asset_start");

	if (root == meshDoc.MemberEnd())
	{
		// #TODO log a helpful message
		return false;
	}

	rapidjson::Value& rootVal = root->value;
	for (auto& member = rootVal.MemberBegin(); member != rootVal.MemberEnd(); ++member)
	{
		std::string memberName = member->name.GetString();
		if (memberName == "meshasset_version")
		{
			const int fileVersion = member->value.GetInt();
			AssertExpr(fileVersion == kAssetImportVersion);
		}
		else
		{
			// Everything else should be a mesh definition, nothing else should be on this
			// layer except meshasset_version.
			rapidjson::Value& meshMemberVal = member->value;
			
			rapidjson::Value::MemberIterator vertexDataSizeMember = meshMemberVal.FindMember("vertex_data_size");
			if (vertexDataSizeMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), "vertex_data_size");
				return false;
			}

			rapidjson::Value::MemberIterator vertexDataMember = meshMemberVal.FindMember("vertex_data");
			if (vertexDataMember == meshMemberVal.MemberEnd())
			{
				LOG_CONSOLE_ARGS("Missing data in [%s] : %s", filePath.GetRelativePath().c_str(), "vertex_data");
				return false;
			}


			//#TODO
			// This is broken somehow. Numbers are not lining up in saved version
			// (i.e last float of first mesh or last float of last mesh are not congruent).
			// Write/Read the index data and try to render this and see what happens.
			MeshData meshData;
			const int vertexDataSize = vertexDataSizeMember->value.GetInt();
			meshData.VertexData.reserve(vertexDataSize);
			for (int i = 0; i < vertexDataSize; ++i)
			{
				meshData.VertexData.push_back(static_cast<float>(vertexDataMember->value[i].GetDouble()));
			}
			test.emplace_back(meshData);
		}
	}

	return true;
}
