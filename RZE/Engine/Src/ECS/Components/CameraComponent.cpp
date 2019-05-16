#include <StdAfx.h>
#include <ECS/Components/CameraComponent.h>

void CameraComponent::OnEditorInspect()
{
	ImGui::Checkbox("Main Camera", &bIsActiveCamera);
	ImGui::Text("Field Of View");
	ImGui::InputFloat("##cameracomponent_fov", &FOV, 0.05f, 0.5f);
	
	ImGui::Text("Near Cull");
	ImGui::InputFloat("##cameracomponent_nearcull", &NearCull, 0.05f, 0.05f);
	ImGui::Text("Far Cull");
	ImGui::InputFloat("##cameracomponent_farcull", &FarCull, 0.05f, 0.05f);

	float* forwardDirValues = const_cast<float*>(&Forward.GetInternalVec().x);
	ImGui::Text("Forward Dir");
	ImGui::DragFloat3("##cameracomponent_forwarddir", forwardDirValues, 0.0005f, -100.0f, 100.0f);
}
