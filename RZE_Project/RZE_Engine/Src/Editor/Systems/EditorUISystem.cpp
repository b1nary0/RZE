#include <StdAfx.h>

#include <imGUI/imgui.h>

#include <Editor/Systems/EditorUISystem.h>

#include <Editor/Components/Windows/EditorLogWindow.h>

EditorUISystem::EditorUISystem(IEntityAdmin* admin)
	: IEntitySystem(admin)
{

}

EditorUISystem::~EditorUISystem()
{

}

void EditorUISystem::Init()
{
	InitImGUI();
}

void EditorUISystem::Update()
{
	
}

void EditorUISystem::ShutDown()
{

}

void EditorUISystem::InitImGUI()
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = RZE_Engine::Get()->GetWindowSettings().GetDimensions().X();
	io.DisplaySize.y = RZE_Engine::Get()->GetWindowSettings().GetDimensions().Y();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}
