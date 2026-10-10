#include <StdAfx.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Quaternion.h>
#include <Utils/Reflect/Reflection.h>

CameraComponent::CameraComponent()
{
	REFLECT_REGISTER_COMPONENT(CameraComponent);
}

Vector3D CameraComponent::GetForward() const
{
	const Quaternion rotation(m_owner->GetTransformComponent()->GetRotation() * MathUtils::ToRadians);
	return (rotation * Vector3D(0.0f, 0.0f, -1.0f)).Normalized();
}

Vector3D CameraComponent::GetUpDir() const
{
	const Quaternion rotation(m_owner->GetTransformComponent()->GetRotation() * MathUtils::ToRadians);
	return (rotation * Vector3D(0.0f, 1.0f, 0.0f)).Normalized();
}

const Matrix4x4& CameraComponent::GetProjectionMatrix() const
{
	return m_projectionMat;
}

const Matrix4x4& CameraComponent::GetViewMatrix() const
{
	return m_viewMat;
}

float CameraComponent::GetFOV() const
{
	return m_fov;
}

float CameraComponent::GetAspectRatio() const
{
	return m_aspectRatio;
}

float CameraComponent::GetNearCull() const
{
	return m_nearCull;
}

float CameraComponent::GetFarCull() const
{
	return m_farCull;
}

float CameraComponent::GetExposureCompensation() const
{
	return m_exposureCompensation;
}

bool CameraComponent::IsActiveCamera() const
{
	return m_isActiveCamera;
}

void CameraComponent::SetFOV(float fov)
{
	AssertExpr(fov > 0.0f);
	m_fov = fov;
}

void CameraComponent::SetAspectRatio(float aspectRatio)
{
	AssertExpr(aspectRatio > 0.0f);
	m_aspectRatio = aspectRatio;
}

void CameraComponent::SetNearCull(float nearCull)
{
	AssertExpr(nearCull > 0.0f);
	m_nearCull = nearCull;
}

void CameraComponent::SetFarCull(float farCull)
{
	m_farCull = farCull;
}

void CameraComponent::SetExposureCompensation(float ev)
{
	m_exposureCompensation = ev;
}

void CameraComponent::SetAsActiveCamera(bool isActiveCamera)
{
	m_isActiveCamera = isActiveCamera;
}

void CameraComponent::Initialize()
{
	RenderCamera renderCam;
	renderCam.Viewport = RenderViewport{ Vector2D(426.0f, 240.0f) };
	renderCam.ClipSpace = GetProjectionMatrix() * GetViewMatrix();

	m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(
		static_cast<U32>(renderCam.Viewport.Size.X()),
		static_cast<U32>(renderCam.Viewport.Size.Y())
	);
	m_renderTarget->Initialize();

	m_previewView = std::make_unique<RenderView>(ERenderViewKind::Secondary);
	m_previewView->Target = m_renderTarget.get();
}

void CameraComponent::OnAddToScene()
{
	// #TODO This wont work long term, just trying to get things rendering with new code
	m_isActiveCamera = true;
	m_aspectRatio = RZE().GetWindowSize().X() / RZE().GetWindowSize().Y();
}

void CameraComponent::Update()
{
	if (IsActiveCamera())
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "A camera without a transform is useless");

		GenerateCameraMatrices(transformComponent->GetPosition());

		{
			// Push data to RenderEngine
			RenderCamera& renderCamera = RZE().GetRenderEngine().GetMainView().Camera;
			renderCamera.Position = transformComponent->GetPosition();
			renderCamera.ClipSpace = GetProjectionMatrix() * GetViewMatrix();

			// Also applies in the editor, whose own camera doesn't carry an exposure
			RZE().GetRenderEngine().GetMainView().ExposureCompensation = m_exposureCompensation;
		}
	}
}

void CameraComponent::GenerateCameraMatrices(const Vector3D& position)
{
	OPTICK_EVENT("GenerateCameraMatrices");
	
	m_projectionMat = Matrix4x4::CreatePerspectiveMatrix(m_fov, m_aspectRatio, m_nearCull, m_farCull);
	m_viewMat = Matrix4x4::CreateViewMatrix(position, position + GetForward(), GetUpDir());
}

void CameraComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	writer.String("CameraComponent");
	writer.StartObject();
	{
		writer.Key("FOV");
		writer.Double(m_fov);

		writer.Key("NearCull");
		writer.Double(m_nearCull);

		writer.Key("FarCull");
		writer.Double(m_farCull);

		writer.Key("ExposureCompensation");
		writer.Double(m_exposureCompensation);
	}
	writer.EndObject();
}

void CameraComponent::Deserialize(const rapidjson::Value& data)
{
	m_fov = data["FOV"].GetFloat();
	m_nearCull = data["NearCull"].GetFloat();
	m_farCull = data["FarCull"].GetFloat();
	// Optional: scenes saved before exposure existed default to 0 EV
	m_exposureCompensation = data.HasMember("ExposureCompensation") ? data["ExposureCompensation"].GetFloat() : 0.0f;
}

void CameraComponent::OnEditorInspect()
{
	ImGui::Checkbox("Main Camera", &m_isActiveCamera);
	
	ImGui::Text("Field Of View");
	ImGui::InputFloat("##cameracomponent_fov", &m_fov, 0.05f, 0.5f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::Text("Near Cull");
	ImGui::InputFloat("##cameracomponent_nearcull", &m_nearCull, 0.05f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::Text("Far Cull");
	ImGui::InputFloat("##cameracomponent_farcull", &m_farCull, 0.05f, 0.05f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::Text("Exposure Compensation (EV)");
	ImGui::DragFloat("##cameracomponent_exposurecompensation", &m_exposureCompensation, 0.05f, -4.0f, 4.0f, "%.2f");

	/* Render camera view */
	{
		// #TODO this is a particularly bad part of not having the right API
		// starting to change state of stuff temporarily - could leak state eventually
		const float prevAspectRatio = m_aspectRatio;
		m_aspectRatio = 240.0f / 144.0f;

		GameObjectComponentPtr<TransformComponent> transfComp = GetOwner()->GetTransformComponent();
		GenerateCameraMatrices(transfComp->GetPosition());

		RenderCamera& renderCam = m_previewView->Camera;
		renderCam.Viewport = RenderViewport { Vector2D( 426.0f, 240.0f ) };
		renderCam.ClipSpace = GetProjectionMatrix() * GetViewMatrix();
		renderCam.Position = transfComp->GetPosition();

		m_previewView->ViewportSize = renderCam.Viewport.Size;
		m_previewView->ExposureCompensation = m_exposureCompensation;

		RZE().GetRenderEngine().RenderSecondaryView("Camera View", *m_previewView);
		m_aspectRatio = prevAspectRatio;

		Rendering::TextureBuffer2DHandle texture = m_renderTarget->GetTargetPlatformObject();

		if (texture.IsValid())
		{
			auto clamp = [](float a, float b, float val)
				{
					if (val > b) val = b;
					else if (val < a) val = a;

					return val;
				};
			float uvbx = clamp(0.0f, 1.0f, renderCam.Viewport.Size.X() / m_renderTarget->GetWidth());
			float uvby = clamp(0.0f, 1.0f, renderCam.Viewport.Size.Y() / m_renderTarget->GetHeight());
			ImGui::Image((ImTextureID)(intptr_t)texture.GetTextureData(), ImVec2(renderCam.Viewport.Size.X(), renderCam.Viewport.Size.Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));
		}
	}
}
