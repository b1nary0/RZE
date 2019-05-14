#include <StdAfx.h>

#include <ECS/Components/TransformComponent.h>

#include <ImGui/imgui.h>

void TransformComponent::OnEditorInspect()
{
	float* values = const_cast<float*>(&Position.GetInternalVec().x);

	ImGui::Text("Position");
	ImGui::DragFloat3("", values, 0.0025f);
}
