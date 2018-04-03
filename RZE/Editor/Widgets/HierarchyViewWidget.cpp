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
			if (ImGui::TreeNode(entity.Name.c_str()))
			{

				ImGui::TreePop();
			}
		}

		ImGui::End();
	}
}
