#include <EditorApp.h>

#include <ImGui/imgui.h>

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp()
{
}

static ImFont* ubuntu = nullptr;
void EditorApp::Initialize()
{
	RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	FilePath ubuntuPath("Assets/Fonts/Ubuntu-Medium.ttf");

	ubuntu = io.Fonts->AddFontFromFileTTF(ubuntuPath.GetAbsolutePath().c_str(), 15);
	io.Fonts->Build();
}

void EditorApp::Start()
{
}

void EditorApp::Update()
{
	static bool bShowDemoWindow = false;
	static bool bShowSceneWindow = false;

	ImGui::PushFont(ubuntu);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 5.0f));
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				RZE().PostExit();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Scene"))
			{
				if (ImGui::Begin("Scene"))
				{
					bShowSceneWindow = true;
				}
				ImGui::End();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("Show ImGui Demo Window"))
			{
				bShowDemoWindow = !bShowDemoWindow;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();

	if (ImGui::BeginPopupContextVoid())
	{
		if (ImGui::MenuItem("Create Entity"))
		{

		}

		ImGui::EndPopup();
	}

	if (bShowDemoWindow)
	{
		ImGui::ShowDemoWindow();
	}

	if (bShowSceneWindow)
	{
		if (ImGui::Begin("Scene", &bShowSceneWindow))
		{
			
		}
		ImGui::End();
	}

	ImGui::PopFont();
}

void EditorApp::ShutDown()
{
}

void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
{
}
