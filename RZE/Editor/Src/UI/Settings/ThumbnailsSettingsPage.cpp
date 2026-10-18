#include <UI/Settings/ThumbnailsSettingsPage.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <Services/ThumbnailRenderer.h>

#include <Asset/AssetPaths.h>

#include <Utils/Platform/Folders.h>

#include <ImGui/imgui.h>

namespace
{
	constexpr double k_refreshIntervalMS = 1000.0;
}

namespace Editor
{
	REGISTER_SETTINGS_PAGE(ThumbnailsSettingsPage);

	ThumbnailsSettingsPage::ThumbnailsSettingsPage()
	{
		m_kinds.push_back({ "Scenes", AssetPaths::GetSceneFolder(), AssetPaths::k_sceneExtension, &Thumbnails::Scenes() });
		m_kinds.push_back({ "Meshes", AssetPaths::GetMeshFolder(), AssetPaths::k_meshExtension, &Thumbnails::Meshes() });
	}

	void ThumbnailsSettingsPage::Draw(EditorContext& context)
	{
		if (!m_hasCounts || m_sinceRefresh.GetElapsedMS<double>() >= k_refreshIntervalMS)
		{
			RefreshCounts();
		}

		ImGui::TextWrapped("Thumbnails render in the background, one at a time, and the status bar shows their progress.");

		for (Kind& kind : m_kinds)
		{
			ImGui::Spacing();
			DrawKind(context, kind);
		}
	}

	void ThumbnailsSettingsPage::RefreshCounts()
	{
		for (Kind& kind : m_kinds)
		{
			kind.Assets = Folders::FindFiles(kind.AssetFolder, kind.Extension);
			kind.Counts = Thumbnails::Survey(kind.Assets, *kind.ImageFolder);
		}

		m_hasCounts = true;
		m_sinceRefresh.Start();
	}

	void ThumbnailsSettingsPage::DrawKind(EditorContext& context, Kind& kind)
	{
		ImGui::PushID(kind.Heading);
		ImGui::SeparatorText(kind.Heading);

		const Thumbnails::Coverage& coverage = kind.Counts;
		if (coverage.GetTotal() == 0)
		{
			ImGui::TextDisabled("None in %s", kind.AssetFolder.GetRelativePath().c_str());
		}
		else
		{
			ImGui::Text("%zu of %zu up to date", coverage.UpToDate, coverage.GetTotal());
			if (coverage.OutOfDate > 0)
			{
				ImGui::SameLine();
				ImGui::TextDisabled("\xC2\xB7");
				ImGui::SameLine();
				ImGui::Text("%zu out of date", coverage.OutOfDate);
			}
			if (coverage.Missing > 0)
			{
				ImGui::SameLine();
				ImGui::TextDisabled("\xC2\xB7");
				ImGui::SameLine();
				ImGui::Text("%zu missing", coverage.Missing);
			}
		}
		ImGui::TextDisabled("In %s", kind.ImageFolder->GetFolder().GetRelativePath().c_str());

		ThumbnailRenderer& renderer = context.GetThumbnails();

		ImGui::BeginDisabled(coverage.UpToDate == coverage.GetTotal());
		if (ImGui::Button("Update"))
		{
			// Up-to-date ones are skipped
			for (const Filepath& asset : kind.Assets)
			{
				renderer.Request(asset);
			}
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Renders the thumbnails that are missing or older than their asset");

		ImGui::SameLine();
		ImGui::BeginDisabled(coverage.GetTotal() == 0);
		if (ImGui::Button("Regenerate All"))
		{
			for (const Filepath& asset : kind.Assets)
			{
				renderer.Request(asset, true);
			}
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Renders every thumbnail again, up to date or not");

		ImGui::PopID();
	}
}
