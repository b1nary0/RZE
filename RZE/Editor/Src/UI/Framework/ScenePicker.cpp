#include <UI/Framework/ScenePicker.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/TexelReadback.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/ViewProjection.h>

namespace Editor
{
	struct ScenePicker::PendingPick
	{
		std::shared_ptr<Rendering::TexelReadback> Result;
		std::shared_ptr<Rendering::TexelReadback> Depth;
		ResultCallback OnResult;

		// The pixel's centre, and the view's camera and size when it was asked for, to turn the depth into a position
		Vector2D Pixel;
		Matrix4x4 ClipSpace;
		Vector2D ViewSize;
	};

	ScenePicker::ScenePicker() = default;
	ScenePicker::~ScenePicker() = default;

	void ScenePicker::RequestPick(U32 x, U32 y, const ResultCallback& onResult)
	{
		m_pending = std::make_unique<PendingPick>();
		m_pending->Result = std::make_shared<Rendering::TexelReadback>();
		m_pending->Depth = std::make_shared<Rendering::TexelReadback>();
		m_pending->OnResult = onResult;

		// The camera as the last scene update left it. The pick renders after this frame's update, which only moves it
		// while the right mouse button's held, so the two are the same for a click or a drop.
		RenderView& view = RZE().GetRenderEngine().GetMainView();
		m_pending->Pixel = Vector2D(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
		m_pending->ClipSpace = view.Camera.ClipSpace;
		m_pending->ViewSize = view.ViewportSize;

		RenderPickRequest pickRequest;
		pickRequest.X = x;
		pickRequest.Y = y;
		pickRequest.Result = m_pending->Result;
		pickRequest.DepthResult = m_pending->Depth;
		view.Pick = pickRequest;
	}

	void ScenePicker::Update()
	{
		// Both are read in the same render command list, the depth last
		if (m_pending == nullptr || !m_pending->Depth->IsReady.load(std::memory_order_acquire))
		{
			return;
		}

		// Moved out before the callback, which may request the next pick
		const std::unique_ptr<PendingPick> pending = std::move(m_pending);

		// The depth target's cleared to the far plane, so that's where nothing is drawn
		std::optional<Vector3D> surfacePosition;
		const float depth = RenderPickRequest::DecodeDepth(pending->Depth->Value);
		if (depth < 1.0f)
		{
			surfacePosition = ViewProjection::Unproject(pending->ClipSpace, pending->ViewSize, pending->Pixel, depth);
		}

		// Pick IDs are the GameObjectID + 1 (RenderComponent::CreateRenderObject)
		const ScenePick pick{ FindObject(pending->Result->Value), surfacePosition, ViewProjection::GetRay(pending->ClipSpace, pending->ViewSize, pending->Pixel) };

		pending->OnResult(pick);
	}

	void ScenePicker::Cancel()
	{
		// The render thread may still fill in the readbacks; they're shared, so dropping them here is safe
		m_pending.reset();
	}

	GameObjectPtr ScenePicker::FindObject(U32 pickID) const
	{
		GameObjectPtr hitObject;
		if (pickID == 0)
		{
			return hitObject;
		}

		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([&hitObject, pickID](GameObjectPtr gameObject)
			{
				if (gameObject->GetID() == pickID - 1)
				{
					hitObject = gameObject;
				}
			}));
		return hitObject;
	}
}
