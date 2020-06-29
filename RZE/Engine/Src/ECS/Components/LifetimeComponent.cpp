#include <StdAfx.h>
#include <ECS/Components/LifetimeComponent.h>

void LifetimeComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	ImGui::Text("Lifetime(ms)");
	ImGui::InputFloat("##lifetimeComponent_lifetimems", &LifetimeMS, 0.05f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
}
