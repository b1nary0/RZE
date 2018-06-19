#include <Widgets/MainMenuWidget.h>

#include <EngineApp.h>

MainMenuWidget::MainMenuWidget()
{

}

MainMenuWidget::~MainMenuWidget()
{

}

void MainMenuWidget::Initialize()
{

}

void MainMenuWidget::Display()
{
	if (ImGui::BeginMainMenuBar())
	{
		DoFileMenu();
		DoSceneMenu();

		ImGui::EndMainMenuBar();
	}
}

void MainMenuWidget::DoFileMenu()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Exit"))
		{
			RZE_Application::RZE().PostExit();
		}

		ImGui::EndMenu();
	}
}

void MainMenuWidget::DoSceneMenu()
{
	if (ImGui::BeginMenu("Scene"))
	{
		if (ImGui::MenuItem("Add Entity"))
		{
			RZE_Application::RZE().GetActiveScene().CreateEntity("DefaultEntityName");
		}

		if (ImGui::BeginMenu("Add Component"))
		{
			if (ImGui::MenuItem("TestComponent"))
			{

			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}
