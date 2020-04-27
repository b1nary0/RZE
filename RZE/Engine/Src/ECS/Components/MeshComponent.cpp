#include <StdAfx.h>
#include <ECS/Components/MeshComponent.h>

void MeshComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	ImGui::Text(ResourcePath.GetRelativePath().c_str());
	if (ImGui::Button("Browse..."))
	{
		FilePath path = RZE_Application::RZE().ShowOpenFilePrompt();
		if (path.IsValid())
		{
			ResourcePath = FilePath(path);
			RZE_Application::RZE().GetActiveScene().GetEntityHandler().OnComponentModified<MeshComponent>(entityID);
		}
	}
}

void MeshComponent::Load(const rapidjson::Value& data)
{
	ResourcePath = FilePath(data["ResourcePath"].GetString());
}

void MeshComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("MeshComponent");
	writer.StartObject();
	{
		writer.Key("ResourcePath");
		writer.String(ResourcePath.GetRelativePath().c_str());
	}
	writer.EndObject();
}
