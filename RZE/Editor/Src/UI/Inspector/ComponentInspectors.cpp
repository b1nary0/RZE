#include <UI/Inspector/ComponentInspectors.h>

#include <unordered_map>
#include <utility>
#include <vector>

namespace Editor
{
	namespace
	{
		struct PendingRegistration
		{
			ComponentInspectors::GetComponentID GetID;
			ComponentInspectors::Inspector Inspector;
		};

		// Function-local statics, so they exist before any registrar uses them
		std::vector<PendingRegistration>& GetPending()
		{
			static std::vector<PendingRegistration> s_pending;
			return s_pending;
		}

		std::unordered_map<GameObjectComponentID, ComponentInspectors::Inspector>& GetInspectors()
		{
			static std::unordered_map<GameObjectComponentID, ComponentInspectors::Inspector> s_inspectors;
			return s_inspectors;
		}
	}

	namespace ComponentInspectors
	{
		void Register(GetComponentID getComponentID, Inspector inspector)
		{
			GetPending().push_back({ getComponentID, std::move(inspector) });
		}

		const Inspector* Find(GameObjectComponentID componentID)
		{
			// Resolves IDs on first use: component type IDs are handed out at runtime
			std::vector<PendingRegistration>& pending = GetPending();
			for (PendingRegistration& registration : pending)
			{
				GetInspectors()[registration.GetID()] = std::move(registration.Inspector);
			}
			pending.clear();

			const auto it = GetInspectors().find(componentID);
			return it != GetInspectors().end() ? &it->second : nullptr;
		}
	}
}
