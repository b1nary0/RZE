#include <doctest/doctest.h>

#include <UI/Settings/SettingsPageRegistry.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace Editor;

namespace
{
	// Registered in the opposite order to their names, so the sort is what puts them in order. UnitTests doesn't link
	// EditorLib with /WHOLEARCHIVE, so these are the only pages it has.
	class ZuluTestPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Zulu Test Page"; }
		void Draw(EditorContext&) override {}
	};

	class AlphaTestPage : public SettingsPage
	{
	public:
		std::string GetName() const override { return "Alpha Test Page"; }
		void Draw(EditorContext&) override {}
	};

	REGISTER_SETTINGS_PAGE(ZuluTestPage);
	REGISTER_SETTINGS_PAGE(AlphaTestPage);

	std::vector<std::string> GetNames(const std::vector<std::unique_ptr<SettingsPage>>& pages)
	{
		std::vector<std::string> names;
		for (const std::unique_ptr<SettingsPage>& page : pages)
		{
			names.push_back(page->GetName());
		}
		return names;
	}
}

TEST_CASE("CreateAll makes one of each registered page, sorted by name")
{
	const std::vector<std::unique_ptr<SettingsPage>> pages = SettingsPageRegistry::CreateAll();

	CHECK(GetNames(pages) == std::vector<std::string>{ "Alpha Test Page", "Zulu Test Page" });
	CHECK(dynamic_cast<AlphaTestPage*>(pages[0].get()) != nullptr);
	CHECK(dynamic_cast<ZuluTestPage*>(pages[1].get()) != nullptr);
}

TEST_CASE("Each CreateAll makes pages of its own")
{
	const std::vector<std::unique_ptr<SettingsPage>> first = SettingsPageRegistry::CreateAll();
	const std::vector<std::unique_ptr<SettingsPage>> second = SettingsPageRegistry::CreateAll();

	REQUIRE(first.size() == second.size());
	for (size_t i = 0; i < first.size(); ++i)
	{
		CHECK(first[i].get() != second[i].get());
	}
}
