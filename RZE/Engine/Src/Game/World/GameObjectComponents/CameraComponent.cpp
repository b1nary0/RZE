#include <StdAfx.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Game/GameScene.h>
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
	return (rotation * WorldAxes::Forward()).Normalized();
}

Vector3D CameraComponent::GetUpDir() const
{
	const Quaternion rotation(m_owner->GetTransformComponent()->GetRotation() * MathUtils::ToRadians);
	return (rotation * WorldAxes::Up()).Normalized();
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

bool CameraComponent::IsMainCamera() const
{
	return m_isMainCamera;
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

void CameraComponent::SetMainCamera(bool isMainCamera)
{
	if (isMainCamera == m_isMainCamera)
	{
		return;
	}

	RZE().GetActiveScene().SetMainCamera(isMainCamera ? GetOwner() : GameObjectPtr());
}

Vector2D CameraComponent::GetPreviewSize()
{
	return Vector2D(426.0f, 240.0f);
}

RenderCamera CameraComponent::CreateRenderCamera(const Vector2D& viewportSize) const
{
	const Vector3D& position = GetOwner()->GetTransformComponent()->GetPosition();

	// Local matrices; the camera's own are for its real viewport
	const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(m_fov, viewportSize.X() / viewportSize.Y(), m_nearCull, m_farCull);
	const Matrix4x4 view = Matrix4x4::CreateViewMatrix(position, position + GetForward(), GetUpDir());

	RenderCamera renderCam;
	renderCam.Viewport = RenderViewport{ viewportSize };
	renderCam.ClipSpace = projection * view;
	renderCam.Position = position;
	return renderCam;
}

void CameraComponent::RenderPreview()
{
	const Vector2D previewSize = GetPreviewSize();

	m_previewView->Camera = CreateRenderCamera(previewSize);
	m_previewView->ViewportSize = previewSize;
	m_previewView->ExposureCompensation = m_exposureCompensation;

	RenderEngine& renderEngine = RZE().GetRenderEngine();
	renderEngine.RenderSecondaryView("Camera View", *m_previewView, renderEngine.GetMainScene());
}

void CameraComponent::Initialize()
{
	const Vector2D previewSize = GetPreviewSize();
	m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(
		static_cast<U32>(previewSize.X()),
		static_cast<U32>(previewSize.Y())
	);
	m_renderTarget->Initialize();

	m_previewView = std::make_unique<RenderView>(ERenderViewKind::Secondary);
	m_previewView->Target = m_renderTarget.get();
}

void CameraComponent::OnAddToScene()
{
	m_aspectRatio = RZE().GetWindowSize().X() / RZE().GetWindowSize().Y();
}

void CameraComponent::Update()
{
	if (!IsMainCamera())
	{
		return;
	}

	// Also applies in the editor, whose own camera doesn't carry an exposure
	RZE().GetRenderEngine().GetMainView().ExposureCompensation = m_exposureCompensation;

	// The Editor's own camera drives the view there instead
	if (RZE().GetActiveScene().DoesMainCameraDriveView())
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "A camera without a transform is useless");

		GenerateCameraMatrices(transformComponent->GetPosition());

		RenderCamera& renderCamera = RZE().GetRenderEngine().GetMainView().Camera;
		renderCamera.Position = transformComponent->GetPosition();
		renderCamera.ClipSpace = GetProjectionMatrix() * GetViewMatrix();
	}
}

void CameraComponent::GenerateCameraMatrices(const Vector3D& position)
{
	OPTICK_EVENT("GenerateCameraMatrices");
	
	m_projectionMat = Matrix4x4::CreatePerspectiveMatrixZeroToOne(m_fov, m_aspectRatio, m_nearCull, m_farCull);
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

		writer.Key("MainCamera");
		writer.Bool(m_isMainCamera);
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
	// Optional too; GameScene::ResolveMainCamera picks one after a load if no camera has it
	const auto mainCamera = data.FindMember("MainCamera");
	m_isMainCamera = mainCamera != data.MemberEnd() && mainCamera->value.IsBool() && mainCamera->value.GetBool();
}

void CameraComponent::DescribeProperties(PropertyBuilder<CameraComponent>& properties)
{
	properties.Bool("Main Camera", &CameraComponent::IsMainCamera, &CameraComponent::SetMainCamera).Exclusive()
		.ToolTip("The camera the game renders through. Only one camera in the scene can be the main one.");
	// The ranges keep the setters' asserts (positive FOV and near cull) from firing
	properties.Float("Field Of View", &CameraComponent::GetFOV, &CameraComponent::SetFOV)
		.Range(1.0f, 179.0f).StepSize(0.5f).DisplayFormat("%.2f").ToolTip("Vertical, in degrees");
	properties.Float("Near Cull", &CameraComponent::GetNearCull, &CameraComponent::SetNearCull)
		.Range(0.001f, 100.0f).StepSize(0.05f).DisplayFormat("%.3f");
	properties.Float("Far Cull", &CameraComponent::GetFarCull, &CameraComponent::SetFarCull)
		.Range(0.01f, 100000.0f).StepSize(1.0f).DisplayFormat("%.2f");
	properties.Float("Exposure Compensation (EV)", &CameraComponent::GetExposureCompensation, &CameraComponent::SetExposureCompensation)
		.Range(-4.0f, 4.0f).StepSize(0.05f).DisplayFormat("%.2f").As(EPropertyWidget::Drag);
}

