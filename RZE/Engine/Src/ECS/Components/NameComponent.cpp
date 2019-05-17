#include <StdAfx.h>
#include <ECS/Components/NameComponent.h>

void NameComponent::OnEditorInspect(Apollo::EntityID entityID)
{
	char name[256];
	memcpy(name, Name.c_str(), 256);
	if (ImGui::InputText("##namecomponent_name", name, 256))
	{
		Name = name;
	}
}
