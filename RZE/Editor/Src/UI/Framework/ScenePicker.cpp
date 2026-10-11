#include <UI/Framework/ScenePicker.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/TexelReadback.h>

namespace Editor
{
	void ScenePicker::RequestPick(U32 x, U32 y, const ResultCallback& onResult)
	{
		m_pendingResult = std::make_shared<Rendering::TexelReadback>();
		m_onResult = onResult;

		RenderPickRequest pickRequest;
		pickRequest.X = x;
		pickRequest.Y = y;
		pickRequest.Result = m_pendingResult;
		RZE().GetRenderEngine().GetMainView().Pick = pickRequest;
	}

	void ScenePicker::Update()
	{
		if (m_pendingResult == nullptr || !m_pendingResult->IsReady.load(std::memory_order_acquire))
		{
			return;
		}

		// Pick IDs are the GameObjectID + 1 (RenderComponent::CreateRenderObject)
		const U32 pickID = m_pendingResult->Value;

		GameObjectPtr hitObject;
		if (pickID != 0)
		{
			RZE().GetActiveScene().ForEachGameObject(
				Functor<void, GameObjectPtr>([&hitObject, pickID](GameObjectPtr gameObject)
				{
					if (gameObject->GetID() == pickID - 1)
					{
						hitObject = gameObject;
					}
				}));
		}

		// Cleared before the callback, which may request the next pick
		const ResultCallback onResult = m_onResult;
		Cancel();

		onResult(hitObject);
	}

	void ScenePicker::Cancel()
	{
		// The render thread may still fill in the readback; it's shared, so dropping it here is safe
		m_pendingResult.reset();
		m_onResult = ResultCallback();
	}
}
