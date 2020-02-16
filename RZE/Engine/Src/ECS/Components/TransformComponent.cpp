#include <StdAfx.h>

#include <ECS/Components/TransformComponent.h>

#include <ImGui/imgui.h>

void TransformComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	float* positionValues = const_cast<float*>(&Position.GetInternalVec().x);
	float* rotationValues = const_cast<float*>(&Rotation.GetInternalVec().x);
	float* scaleValues = const_cast<float*>(&Scale.GetInternalVec().x);


	// #TODO(Modify DragFloat3 to only register on mouse movement to avoid falloff when dragging)


	ImGui::Text("Position");
	ImGui::DragFloat3("##transfcomp_position", positionValues, 0.005f);
	ImGui::Text("Rotation");
	ImGui::DragFloat3("##transfcomp_rotation", rotationValues, 0.05f);
	ImGui::Text("Scale");
	ImGui::DragFloat3("##transfcomp_scale", scaleValues, 0.005f);
}