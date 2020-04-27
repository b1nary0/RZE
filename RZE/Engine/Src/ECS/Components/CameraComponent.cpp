#include <StdAfx.h>
#include <ECS/Components/CameraComponent.h>

void CameraComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	ImGui::Checkbox("Main Camera", &bIsActiveCamera);

	if (ImGui::IsItemEdited())
	{
		RZE_Application::RZE().GetActiveScene().GetEntityHandler().OnComponentModified<CameraComponent>(entityID);
	}

	ImGui::Text("Field Of View");
	ImGui::InputFloat("##cameracomponent_fov", &FOV, 0.05f, 0.5f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
	
	ImGui::Text("Near Cull");
	ImGui::InputFloat("##cameracomponent_nearcull", &NearCull, 0.05f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::Text("Far Cull");
	ImGui::InputFloat("##cameracomponent_farcull", &FarCull, 0.05f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);

	float* forwardDirValues = const_cast<float*>(&Forward.GetInternalVec().x);
	ImGui::Text("Look At");
	ImGui::DragFloat3("##cameracomponent_forwarddir", forwardDirValues, 0.005f, -100.0f, 100.0f);
}

void CameraComponent::Load(const rapidjson::Value& data)
{
	FOV = data["FOV"].GetFloat();
	NearCull = data["NearCull"].GetFloat();
	FarCull = data["FarCull"].GetFloat();
	Forward = Vector3D(data["Forward"][0].GetFloat(), data["Forward"][1].GetFloat(), data["Forward"][2].GetFloat());
	UpDir = Vector3D(data["UpDir"][0].GetFloat(), data["UpDir"][1].GetFloat(), data["UpDir"][2].GetFloat());
}
