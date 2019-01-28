#include <Widgets/MainMenuWidget.h>

#include <EngineApp.h>

#include <random>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

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
			srand(static_cast<unsigned int>(time(NULL)));
			for (int i = 0; i < 3000; ++i)
			{
				Apollo::EntityID entity = RZE_Application::RZE().GetActiveScene().CreateEntity("DefaultEntityName");
				RZE_Application::RZE().GetActiveScene().GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Assets/3D/FW190/FW190.obj"));

				TransformComponent* const transf = RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponent<TransformComponent>(entity);
				transf->Position = Vector3D((rand() + 1) % 25, (rand() + 1) % 25, (rand() + 1) % 25);
			}
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
