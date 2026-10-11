#include <UI/Framework/PanelRegistry.h>

namespace Editor
{
	namespace
	{
		// A function-local static, so it's constructed before the first registrar uses it whatever the
		// static initialization order
		std::vector<PanelRegistration>& GetRegistrations()
		{
			static std::vector<PanelRegistration> s_registrations;
			return s_registrations;
		}
	}

	namespace PanelRegistry
	{
		void Register(const PanelRegistration& registration)
		{
			GetRegistrations().push_back(registration);
		}

		const std::vector<PanelRegistration>& GetAll()
		{
			return GetRegistrations();
		}
	}
}
