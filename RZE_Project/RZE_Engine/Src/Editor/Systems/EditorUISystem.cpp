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

	mRelevantComponents.AddFilterType<EditorLogWindow>();
}

void EditorUISystem::Update()
{
	std::vector<IEntity*> entities;
	mRelevantComponents.FilterEachOf(mAdmin->GetEntities(), entities);

	for (auto& entity : entities)
	{
		EditorLogWindow* logWindow = entity->GetComponent<EditorLogWindow>();
		AssertNotNull(logWindow);

		ImGui::SetNextWindowPos(ImVec2(logWindow->GetPosition().X(), logWindow->GetPosition().Y()));
		ImGui::SetNextWindowSize(ImVec2(logWindow->GetDimensions().X(), logWindow->GetDimensions().Y()));
		ImGui::Begin(logWindow->GetName().c_str());

		const std::deque<std::string>& items = logWindow->GetItems();
		for (size_t itemIdx = 0; itemIdx < items.size(); itemIdx++)
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), items[itemIdx].c_str());
		}
		ImGui::SetScrollHere();

		ImGui::End();
	}
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
