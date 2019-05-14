#include <StdAfx.h>

#include <ECS/Components/TransformComponent.h>

#include <ImGui/imgui.h>

void TransformComponent::OnEditorInspect()
{
	float* positionValues = const_cast<float*>(&Position.GetInternalVec().x);
	float* rotationValues = const_cast<float*>(&Rotation.GetInternalQuat().x);
	float* scaleValues = const_cast<float*>(&Scale.GetInternalVec().x);

	ImGui::Text("Position");
	ImGui::DragFloat3("", positionValues, 0.005f);
	ImGui::Text("Rotation");
	ImGui::DragFloat3("", rotationValues, 0.005f);
	ImGui::Text("Scale");
	ImGui::DragFloat3("", scaleValues, 0.005f);
}
