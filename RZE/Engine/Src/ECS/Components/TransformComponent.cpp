#include <StdAfx.h>

#include <ECS/Components/TransformComponent.h>

#include <ImGui/imgui.h>

void TransformComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	float* positionValues = const_cast<float*>(&Position.GetInternalVec().x);
	float* rotationValues = const_cast<float*>(&Rotation.GetInternalVec().x);
	float* scaleValues = const_cast<float*>(&Scale.GetInternalVec().x);

	ImGui::Text("Position");
	ImGui::InputFloat3("##transfcomp_position", positionValues, 2);
	ImGui::Text("Rotation");
	ImGui::InputFloat3("##transfcomp_rotation", rotationValues, 2);
	ImGui::Text("Scale");
	ImGui::InputFloat3("##transfcomp_scale", scaleValues, 2);
}
