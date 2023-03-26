#include <MeshEditor.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Platform/CmdLine.h>

#include <imGUI/imgui.h>

#include "Game/StaticMeshResource.h"

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

		std::string_view asset;
		AssertExpr(CmdLine::Arguments::Get("-asset", asset));

		RZE().GetActiveScene().Deserialize(Filepath("Assets/Scenes/Default.scene"));

		std::string assetStr = std::string(asset);
		GameObjectPtr assetObject = RZE().GetActiveScene().AddGameObject(assetStr);
		GameObjectComponentPtr<RenderComponent> renderComponent = assetObject->AddComponent<RenderComponent>();

		ResourceHandle assetResource = RZE().GetResourceHandler().LoadResource<StaticMeshResource>(Filepath("ProjectData/Mesh/" + assetStr));
		renderComponent->SetMeshResource(assetResource);
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
		Rendering::RenderTargetTexture* const pRTT = RZE().GetApplication().GetRTT();
		Rendering::TextureBuffer2DHandle texture = pRTT->GetTargetPlatformObject();

		auto clamp = [](float a, float b, float val)
		{
			if (val > b) val = b;
			else if (val < a) val = a;

			return val;
		};

		// #TODO::Josh
		// don't use windowdims
		float uvbx = clamp(0.0f, 1.0f, windowDims.X() / pRTT->GetWidth());
		float uvby = clamp(0.0f, 1.0f, windowDims.Y() / pRTT->GetHeight());
		ImGui::Image(texture.GetTextureData(), ImVec2(windowDims.X(), windowDims.Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));
		ImGui::End();
	}

	void MeshEditorApp::ShutDown()
	{
		RZE_Application::ShutDown();
	}

	void MeshEditorApp::ParseArguments(char** arguments, int count)
	{
		CmdLine::Arguments::Initialize(arguments, count);
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
