#pragma once

#include <Utils/PrimitiveDefs.h>

#include <functional>
#include <memory>
#include <vector>

namespace Editor
{
	class EditorSelection;
	class SceneDocument;

	// Before the scene unloads (EditorContext::ResetSceneState), while its objects can still be touched
	struct SceneUnloadingEvent {};
	// A load (or New) succeeded; the scene and the editor camera are in
	struct SceneLoadedEvent { const SceneDocument& Document; };
	// The scene was written to its file
	struct SceneSavedEvent { const SceneDocument& Document; };
	// A command flush ran at least one command, so the scene may differ from its file
	struct SceneModifiedEvent {};
	// Once per EditorSelection call that changed it
	struct SelectionChangedEvent { const EditorSelection& Selection; };

	namespace Internal
	{
		struct EventListeners;
	}

	// Keeps a listener subscribed until it's destroyed or Reset, so a listener can't outlive its owner. Safe to
	// destroy after the EditorEvents it came from.
	class EventSubscription
	{
	public:
		EventSubscription() = default;
		~EventSubscription() { Reset(); }

		EventSubscription(EventSubscription&& other) noexcept;
		EventSubscription& operator=(EventSubscription&& other) noexcept;
		EventSubscription(const EventSubscription&) = delete;
		EventSubscription& operator=(const EventSubscription&) = delete;

	public:
		void Reset();

	private:
		friend class EditorEvents;
		EventSubscription(const std::shared_ptr<Internal::EventListeners>& listeners, size_t eventType, U64 listenerID);

	private:
		std::weak_ptr<Internal::EventListeners> m_listeners;
		size_t m_eventType = 0;
		U64 m_listenerID = 0;
	};

	// Tells whoever's interested about editor-wide changes, so they don't have to poll EditorContext. Owned by
	// EditorContext. Main thread only (Publish asserts it).
	//
	// Events are delivered synchronously, as they're published, which can be in the middle of drawing (e.g. a click
	// in the Scene panel changing the selection), so listeners must not call ImGui. A listener subscribed during a
	// publish gets the next event, not the current one; one unsubscribed during a publish isn't called again.
	class EditorEvents
	{
	public:
		EditorEvents();
		~EditorEvents() = default;

		EditorEvents(const EditorEvents&) = delete;
		EditorEvents& operator=(const EditorEvents&) = delete;

	public:
		template <typename TEvent>
		[[nodiscard]] EventSubscription Subscribe(std::function<void(const TEvent&)> listener)
		{
			return Subscribe(GetEventType<TEvent>(),
				[listener = std::move(listener)](const void* event) { listener(*static_cast<const TEvent*>(event)); });
		}

		template <typename TEvent>
		void Publish(const TEvent& event)
		{
			Publish(GetEventType<TEvent>(), &event);
		}

	private:
		// A small number per event type, assigned on first use; no RTTI
		static size_t NextEventType();
		template <typename TEvent>
		static size_t GetEventType()
		{
			static const size_t eventType = NextEventType();
			return eventType;
		}

		EventSubscription Subscribe(size_t eventType, std::function<void(const void*)> listener);
		void Publish(size_t eventType, const void* event);

	private:
		// Shared with each subscription, so one destroyed after this doesn't touch freed memory
		std::shared_ptr<Internal::EventListeners> m_listeners;
	};
}
