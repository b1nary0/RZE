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
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				RZE_Application::RZE().PostExit();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}
