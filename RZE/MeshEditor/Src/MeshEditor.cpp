#include <MeshEditor.h>

#include <Graphics/RenderEngine.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <imGUI/imgui.h>

#include <Graphics/RenderStages/ImGuiRenderStage.h>

namespace MeshEditor
{
	void MeshEditorApp::Initialize()
	{
		RZE_Application::Initialize();

		constexpr bool isWithEditor = true;
		RZE().GetRenderEngine().AddRenderStage<ImGuiRenderStage>(isWithEditor);

		const Vector2D& windowDims = GetWindow()->GetDimensions();
		CreateRenderTarget(windowDims);
		RZE().GetRenderEngine().SetRenderTarget(m_renderTarget.get());
	}

	void MeshEditorApp::Start()
	{
		RZE_Application::Start();

		RZE().GetActiveScene().Deserialize(Filepath("Assets/Scenes/Default.scene"));
	}

	void MeshEditorApp::Update()
	{
		RZE_Application::Update();

		Vector2D windowDims = GetWindow()->GetDimensions();
		ImGui::SetNextWindowSize(ImVec2(windowDims.X(), windowDims.Y()));
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.0f));

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		bool show = true;
		ImGui::Begin("MeshEditor", &show, windowFlags);
		ImGui::Text("This is MeshEditor");
		ImGui::End();
	}

	void MeshEditorApp::ShutDown()
	{
		RZE_Application::ShutDown();
	}

	void MeshEditorApp::ParseArguments(char** arguments, int count)
	{
		RZE_Application::ParseArguments(arguments, count);
	}

	void MeshEditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
		RZE_Application::RegisterInputEvents(inputHandler);
	}

	bool MeshEditorApp::ProcessInput(const InputHandler& handler)
	{
		return RZE_Application::ProcessInput(handler);
	}

	void MeshEditorApp::OnWindowResize(const Vector2D& newSize)
	{
	}

	void MeshEditorApp::CreateRenderTarget(const Vector2D& dimensions)
	{
		m_renderTarget.reset();

		m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(static_cast<U32>(dimensions.X()), static_cast<U32>(dimensions.Y()));
		m_renderTarget->Initialize();
	}
}
