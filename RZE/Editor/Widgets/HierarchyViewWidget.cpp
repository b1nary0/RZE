#include <Widgets/HierarchyViewWidget.h>

#include <RZE.h>
#include <EditorApp.h>

HierarchyViewWidget::HierarchyViewWidget()
{

}

HierarchyViewWidget::~HierarchyViewWidget()
{

}

void HierarchyViewWidget::Initialize()
{

}

void HierarchyViewWidget::Display()
{
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.20f, 250.0f));
	ImGui::SetNextWindowPos(ImVec2(0.0f, 20.0f));
	if (ImGui::Begin("Hierarchy View", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
	{
		std::vector<GameScene::SceneEntryTemp> entities = RZE_Application::RZE().GetActiveScene().GetSceneEntries();
		for (auto& entity : entities)
		{
			if (ImGui::TreeNodeEx(entity.Name.c_str(), ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Framed))
			{
				Apollo::EntityHandler::ComponentNameList componentNames;
				RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponentNames(entity.ID, componentNames);
				for (auto& componentName : componentNames)
				{
					ImGui::Selectable(componentName.c_str());
				}
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}
}
