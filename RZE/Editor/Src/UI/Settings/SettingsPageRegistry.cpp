#include <UI/Settings/SettingsPageRegistry.h>

#include <algorithm>

namespace Editor
{
	namespace
	{
		// A function-local static, so it's constructed before the first registrar uses it whatever the
		// static initialization order
		std::vector<SettingsPageRegistry::CreateFunction>& GetRegistrations()
		{
			static std::vector<SettingsPageRegistry::CreateFunction> s_registrations;
			return s_registrations;
		}
	}

	namespace SettingsPageRegistry
	{
		void Register(CreateFunction create)
		{
			GetRegistrations().push_back(create);
		}

		std::vector<std::unique_ptr<SettingsPage>> CreateAll()
		{
			std::vector<std::unique_ptr<SettingsPage>> pages;
			for (CreateFunction create : GetRegistrations())
			{
				pages.push_back(create());
			}

			std::sort(pages.begin(), pages.end(), [](const std::unique_ptr<SettingsPage>& a, const std::unique_ptr<SettingsPage>& b)
				{
					return a->GetName() < b->GetName();
				});

			return pages;
		}
	}
}
