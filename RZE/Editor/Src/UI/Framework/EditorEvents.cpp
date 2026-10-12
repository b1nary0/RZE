#include <UI/Framework/EditorEvents.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>

namespace Editor
{
	namespace Internal
	{
		struct EventListeners
		{
			struct Listener
			{
				U64 ID = 0;
				// Shared, so a publish can hold on to it cheaply while the list may move
				std::shared_ptr<const std::function<void(const void*)>> Callback;
				// Set when unsubscribed during a publish; swept out once the publish finishes
				bool IsRemoved = false;
			};

			void Remove(size_t eventType, U64 listenerID)
			{
				std::vector<Listener>& listeners = ListenersByType[eventType];
				const auto it = std::find_if(listeners.begin(), listeners.end(),
					[listenerID](const Listener& listener) { return listener.ID == listenerID; });
				if (it == listeners.end())
				{
					return;
				}

				if (PublishDepth > 0)
				{
					// The publish loop is indexing into this list, so it can't shrink yet
					it->IsRemoved = true;
					HasRemoved = true;
				}
				else
				{
					listeners.erase(it);
				}
			}

			std::vector<std::vector<Listener>> ListenersByType;
			U64 NextListenerID = 1;
			int PublishDepth = 0;
			bool HasRemoved = false;
		};
	}

	EventSubscription::EventSubscription(const std::shared_ptr<Internal::EventListeners>& listeners, size_t eventType, U64 listenerID)
		: m_listeners(listeners)
		, m_eventType(eventType)
		, m_listenerID(listenerID)
	{
	}

	EventSubscription::EventSubscription(EventSubscription&& other) noexcept
		: m_listeners(std::move(other.m_listeners))
		, m_eventType(other.m_eventType)
		, m_listenerID(other.m_listenerID)
	{
		other.m_listenerID = 0;
	}

	EventSubscription& EventSubscription::operator=(EventSubscription&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_listeners = std::move(other.m_listeners);
			m_eventType = other.m_eventType;
			m_listenerID = other.m_listenerID;
			other.m_listenerID = 0;
		}
		return *this;
	}

	void EventSubscription::Reset()
	{
		if (const std::shared_ptr<Internal::EventListeners> listeners = m_listeners.lock())
		{
			if (m_listenerID != 0)
			{
				listeners->Remove(m_eventType, m_listenerID);
			}
		}

		m_listeners.reset();
		m_listenerID = 0;
	}

	EditorEvents::EditorEvents()
		: m_listeners(std::make_shared<Internal::EventListeners>())
	{
	}

	size_t EditorEvents::NextEventType()
	{
		static size_t nextEventType = 0;
		return nextEventType++;
	}

	EventSubscription EditorEvents::Subscribe(size_t eventType, std::function<void(const void*)> listener)
	{
		// No main-thread check here: EditorContext's members subscribe while it's constructed, before the engine has
		// set up MainThreadDispatcher. Publish checks.
		if (m_listeners->ListenersByType.size() <= eventType)
		{
			m_listeners->ListenersByType.resize(eventType + 1);
		}

		const U64 listenerID = m_listeners->NextListenerID++;
		m_listeners->ListenersByType[eventType].push_back({ listenerID, std::make_shared<const std::function<void(const void*)>>(std::move(listener)) });
		return EventSubscription(m_listeners, eventType, listenerID);
	}

	void EditorEvents::Publish(size_t eventType, const void* event)
	{
		AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

		Internal::EventListeners& listeners = *m_listeners;
		if (listeners.ListenersByType.size() <= eventType)
		{
			return;
		}

		++listeners.PublishDepth;
		{
			// Index-based and bounded by the count at the start: a listener may subscribe another, which can grow
			// (and move) the list, and shouldn't get this event
			const size_t count = listeners.ListenersByType[eventType].size();
			for (size_t listenerIndex = 0; listenerIndex < count; ++listenerIndex)
			{
				const Internal::EventListeners::Listener& listener = listeners.ListenersByType[eventType][listenerIndex];
				if (!listener.IsRemoved)
				{
					// Held, so it survives the list moving while it runs
					const std::shared_ptr<const std::function<void(const void*)>> callback = listener.Callback;
					(*callback)(event);
				}
			}
		}
		--listeners.PublishDepth;

		if (listeners.PublishDepth == 0 && listeners.HasRemoved)
		{
			for (std::vector<Internal::EventListeners::Listener>& typeListeners : listeners.ListenersByType)
			{
				typeListeners.erase(std::remove_if(typeListeners.begin(), typeListeners.end(),
					[](const Internal::EventListeners::Listener& listener) { return listener.IsRemoved; }), typeListeners.end());
			}
			listeners.HasRemoved = false;
		}
	}
}
