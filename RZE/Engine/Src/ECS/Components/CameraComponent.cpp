#include <StdAfx.h>
#include <ECS/Components/CameraComponent.h>

void CameraComponent::OnEditorInspect()
{
	ImGui::Checkbox("Main Camera", &bIsActiveCamera);
	ImGui::Text("Field Of View");
	ImGui::InputFloat("", &FOV, 0.05f, 0.5f);
	
	ImGui::Separator();

	ImGui::Text("Near Cull");
	ImGui::InputFloat("", &NearCull, 0.5f, 0.5f);
	ImGui::Text("Far Cull");
	ImGui::InputFloat("", &FarCull, 0.5f, 0.5f);

	ImGui::Separator();

	float* forwardDirValues = const_cast<float*>(&Forward.GetInternalVec().x);
	ImGui::Text("Forward Dir");
	ImGui::DragFloat3(" ", forwardDirValues, 0.0005f, -100.0f, 100.0f);
}
