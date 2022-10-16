#include <StdAfx.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>

#include <Utils/DebugUtils/Debug.h>

const Vector3D& CameraComponent::GetLookAt() const
{
	return m_lookAt;
}

const Vector3D& CameraComponent::GetUpDir() const
{
	return m_upDir;
}

const Vector3D& CameraComponent::GetForward() const
{
	return m_forward;
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

bool CameraComponent::IsActiveCamera() const
{
	return m_isActiveCamera;
}

void CameraComponent::SetLookAt(const Vector3D& lookAt)
{
	m_lookAt = lookAt;
}

void CameraComponent::SetUpDir(const Vector3D& upDir)
{
	m_upDir = upDir;
}

void CameraComponent::SetForward(const Vector3D& forward)
{
	m_forward = forward;
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

void CameraComponent::SetAsActiveCamera(bool isActiveCamera)
{
	m_isActiveCamera = isActiveCamera;
}

void CameraComponent::OnAddToScene()
{
	// #TODO This wont work long term, just trying to get things rendering with new code
	m_isActiveCamera = true;
	m_aspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();
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
			RenderCamera& renderCamera = RZE::GetRenderEngine().GetCamera();
			renderCamera.Position = transformComponent->GetPosition();
			renderCamera.ClipSpace = GetProjectionMatrix() * GetViewMatrix();
		}
	}
}

void CameraComponent::GenerateCameraMatrices(const Vector3D& position)
{
	OPTICK_EVENT("GenerateCameraMatrices");
	
	m_projectionMat = Matrix4x4::CreatePerspectiveMatrix(m_fov, m_aspectRatio, m_nearCull, m_farCull);
	m_viewMat = Matrix4x4::CreateViewMatrix(position, position  + m_forward, m_upDir);
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

		writer.Key("Forward");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_forward[i]);
			}
		}
		writer.EndArray();

		writer.Key("UpDir");
		writer.StartArray();
		{
			for (int i = 0; i < 3; ++i)
			{
				writer.Double(m_upDir[i]);
			}
		}
		writer.EndArray();
	}
	writer.EndObject();
}

void CameraComponent::Deserialize(const rapidjson::Value& data)
{
	m_fov = data["FOV"].GetFloat();
	m_nearCull = data["NearCull"].GetFloat();
	m_farCull = data["FarCull"].GetFloat();
	m_forward = Vector3D(data["Forward"][0].GetFloat(), data["Forward"][1].GetFloat(), data["Forward"][2].GetFloat());
	m_upDir = Vector3D(data["UpDir"][0].GetFloat(), data["UpDir"][1].GetFloat(), data["UpDir"][2].GetFloat());
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

	float* forwardDirValues = const_cast<float*>(&m_forward.GetInternalVec().x);
	ImGui::Text("Look At");
	ImGui::DragFloat3("##cameracomponent_forwarddir", forwardDirValues, 0.005f, -100.0f, 100.0f);
}
