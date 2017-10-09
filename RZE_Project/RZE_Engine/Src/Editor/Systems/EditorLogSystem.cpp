#include <StdAfx.h>

#include <Editor/Systems/EditorLogSystem.h>

#include <Editor/Components/Windows/EditorLogWindow.h>

#include <imGUI/imgui.h>

EditorLogSystem::EditorLogSystem(IEntityAdmin* admin)
	: IEntitySystem(admin)
{
}

EditorLogSystem::~EditorLogSystem()
{
}

void EditorLogSystem::Init()
{
	mRelevantComponents.AddFilterType<EditorLogWindow>();
}

void EditorLogSystem::Update()
{
	std::vector<IEntity*> entities;
	mRelevantComponents.FilterAtLeast(mAdmin->GetEntities(), entities);

	for (auto& entity : mAdmin->GetEntities())
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
}

void EditorLogSystem::ShutDown()
{
}
