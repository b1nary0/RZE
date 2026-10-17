#include <UI/Framework/ScenePicker.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/TexelReadback.h>

#include <Utils/Math/ViewProjection.h>

namespace Editor
{
	void ScenePicker::RequestPick(U32 x, U32 y, const ResultCallback& onResult)
	{
		m_pendingResult = std::make_shared<Rendering::TexelReadback>();
		m_pendingDepth = std::make_shared<Rendering::TexelReadback>();
		m_onResult = onResult;

		// The camera as the last scene update left it. The pick renders after this frame's update, which only moves it
		// while the right mouse button's held, so the two are the same for a click or a drop.
		RenderView& view = RZE().GetRenderEngine().GetMainView();
		m_pixel = Vector2D(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
		m_clipSpace = view.Camera.ClipSpace;
		m_viewSize = view.ViewportSize;

		RenderPickRequest pickRequest;
		pickRequest.X = x;
		pickRequest.Y = y;
		pickRequest.Result = m_pendingResult;
		pickRequest.DepthResult = m_pendingDepth;
		view.Pick = pickRequest;
	}

	void ScenePicker::Update()
	{
		// Both are read in the same render command list, the depth last
		if (m_pendingResult == nullptr || !m_pendingDepth->IsReady.load(std::memory_order_acquire))
		{
			return;
		}

		// The depth target's cleared to the far plane, so that's where nothing is drawn
		std::optional<Vector3D> surfacePosition;
		const float depth = RenderPickRequest::DecodeDepth(m_pendingDepth->Value);
		if (depth < 1.0f)
		{
			surfacePosition = ViewProjection::Unproject(m_clipSpace, m_viewSize, m_pixel, depth);
		}

		// Pick IDs are the GameObjectID + 1 (RenderComponent::CreateRenderObject)
		const ScenePick pick{ FindObject(m_pendingResult->Value), surfacePosition, ViewProjection::GetRay(m_clipSpace, m_viewSize, m_pixel) };

		// Cleared before the callback, which may request the next pick
		const ResultCallback onResult = m_onResult;
		Cancel();

		onResult(pick);
	}

	void ScenePicker::Cancel()
	{
		// The render thread may still fill in the readbacks; they're shared, so dropping them here is safe
		m_pendingResult.reset();
		m_pendingDepth.reset();
		m_onResult = ResultCallback();
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
