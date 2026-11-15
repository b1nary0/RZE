#include <UI/FrameStatsView.h>

#include <UI/Framework/EditorUI.h>

#include <RZE.h>

#include <EngineCore/Timing/FramePacer.h>

#include <Graphics/RenderEngine.h>

#include <Rendering/Renderer.h>

#include <Utils/DebugUtils/DebugConfig.h>
#include <Utils/StatsWindow.h>
#include <Utils/StringUtils.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace Editor
{
	namespace
	{
		// The widest each kind of figure usually gets, which fixes its column's width. A wider one still shows, and only
		// pushes what's after it along.
#ifdef RZE_RENDER_STATS
		constexpr char k_widestMS[] = "000.00";
		// StringUtils::FormatCount's widest
		constexpr char k_widestCount[] = "00.0M";
#endif
		// The label tables' values: the widest of the above
		constexpr char k_widestValue[] = "0000.00";
		// The label tables' units, after their values. The probe table ends in an empty column this wide too, so its
		// figures line up with the values above and below it.
		constexpr char k_widestUnit[] = "ms";

		// The bar's line: the frame time, frames a second, idle time and render thread time
		constexpr char k_widestBarMS[] = "00.0";
		constexpr char k_widestBarRate[] = "0000";

		// Between the figure tables' columns, wider than ImGui's so headers like "Vertices" and "In view" don't run together
		constexpr float k_figuresCellPadding = 6.0f;

		// Every table spans the panel: its first column takes what the figures leave, so they sit against the right edge
		constexpr ImGuiTableFlags k_tableFlags = ImGuiTableFlags_SizingFixedFit;

		// UTF-8, spelled out so they don't depend on the source file's encoding
		constexpr char k_dot[] = " \xC2\xB7 ";
#ifdef RZE_RENDER_STATS
		// In the GPU's columns: what it can't count, and what it hasn't counted yet
		constexpr char k_notCounted[] = "\xE2\x80\x94";
		constexpr char k_counting[] = "\xE2\x80\xA6";
#endif

		std::string FormatFixed(double value, int decimals)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.*f", decimals, value);
			return text;
		}

		std::string FormatMS(double milliseconds)
		{
			return FormatFixed(milliseconds, 2);
		}

		// text against the right edge of a slot as wide as widestText starting at x, or from x if it's wider; returns
		// where it ends
		float DrawFigure(ImDrawList& drawList, float x, float y, ImU32 colour, const std::string& text, const char* widestText)
		{
			const float textWidth = ImGui::CalcTextSize(text.c_str()).x;
			const float left = std::max(x, x + ImGui::CalcTextSize(widestText).x - textWidth);
			drawList.AddText(ImVec2(left, y), colour, text.c_str());
			return left + textWidth;
		}

		float DrawWords(ImDrawList& drawList, float x, float y, ImU32 colour, const char* text)
		{
			drawList.AddText(ImVec2(x, y), colour, text);
			return x + ImGui::CalcTextSize(text).x;
		}

		bool BeginFiguresTable(const char* id, int columnCount, ImGuiTableFlags extraFlags)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(k_figuresCellPadding, ImGui::GetStyle().CellPadding.y));
			const bool isOpen = ImGui::BeginTable(id, columnCount, k_tableFlags | extraFlags);
			ImGui::PopStyleVar();
			return isOpen;
		}

#ifdef RZE_RENDER_STATS
		// The GPU's counts for a cell: the count, or why there isn't one
		std::string FormatGPUCount(const RenderStats& stats, double count)
		{
			if (!stats.CanCountGPUStatistics)
			{
				return k_notCounted;
			}
			if (!stats.HasGPUStatistics)
			{
				return k_counting;
			}
			return StringUtils::FormatCount(count);
		}

		// One cell of a table of figures, against its right edge
		void FigureCell(const std::string& text)
		{
			ImGui::TableNextColumn();
			EditorUI::TextAlignedRight(text.c_str());
		}

		// A figure table's header row: the first column's name on the left, the figures' against their right edges, all
		// greyed, rather than ImGui's header row
		void FiguresHeaderRow(int columnCount)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
			for (int column = 0; column < columnCount; ++column)
			{
				ImGui::TableSetColumnIndex(column);
				if (column == 0)
				{
					ImGui::TextDisabled("%s", ImGui::TableGetColumnName(column));
				}
				else
				{
					EditorUI::TextAlignedRight(ImGui::TableGetColumnName(column), true);
				}
			}
		}

		void DrawStageTable(const RenderStats& stats)
		{
			constexpr int k_columnCount = 9;
			if (!BeginFiguresTable("##Stages", k_columnCount, ImGuiTableFlags_RowBg))
			{
				return;
			}

			ImGui::TableSetupColumn("Stage", ImGuiTableColumnFlags_WidthStretch);
			EditorUI::SetupFiguresColumn("Record ms", k_widestMS);
			EditorUI::SetupFiguresColumn("GPU ms", k_widestMS);
			EditorUI::SetupFiguresColumn("Draws", k_widestCount);
			EditorUI::SetupFiguresColumn("Instances", k_widestCount);
			EditorUI::SetupFiguresColumn("Triangles", k_widestCount);
			EditorUI::SetupFiguresColumn("Vertices", k_widestCount);
			EditorUI::SetupFiguresColumn("In view", k_widestCount);
			EditorUI::SetupFiguresColumn("Pixels", k_widestCount);
			FiguresHeaderRow(k_columnCount);

			RenderStageStats total;
			for (const RenderStageStats& stage : stats.Stages)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(stage.Name.c_str());
				FigureCell(FormatMS(stage.TimeMS));
				FigureCell(FormatMS(stage.GPUTimeMS));
				FigureCell(StringUtils::FormatCount(stage.DrawCalls));
				FigureCell(StringUtils::FormatCount(stage.Instances));
				FigureCell(FormatGPUCount(stats, stage.GPUStatistics.TrianglesSubmitted));
				FigureCell(FormatGPUCount(stats, stage.GPUStatistics.VerticesShaded));
				FigureCell(FormatGPUCount(stats, stage.GPUStatistics.TrianglesInView));
				FigureCell(FormatGPUCount(stats, stage.GPUStatistics.PixelsShaded));

				total.TimeMS += stage.TimeMS;
				total.GPUTimeMS += stage.GPUTimeMS;
				total.DrawCalls += stage.DrawCalls;
				total.Instances += stage.Instances;
				total.GPUStatistics += stage.GPUStatistics;
			}

			// The stages' sums, set apart like the header
			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableHeaderBg));
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Total");
			FigureCell(FormatMS(total.TimeMS));
			FigureCell(FormatMS(total.GPUTimeMS));
			FigureCell(StringUtils::FormatCount(total.DrawCalls));
			FigureCell(StringUtils::FormatCount(total.Instances));
			FigureCell(FormatGPUCount(stats, total.GPUStatistics.TrianglesSubmitted));
			FigureCell(FormatGPUCount(stats, total.GPUStatistics.VerticesShaded));
			FigureCell(FormatGPUCount(stats, total.GPUStatistics.TrianglesInView));
			FigureCell(FormatGPUCount(stats, total.GPUStatistics.PixelsShaded));

			ImGui::EndTable();
		}

		void DrawProbeTable(const RenderWorkCounts& work)
		{
			constexpr int k_columnCount = 5;
			if (!BeginFiguresTable("##Probes", k_columnCount, ImGuiTableFlags_None))
			{
				return;
			}

			ImGui::TableSetupColumn("Probes", ImGuiTableColumnFlags_WidthStretch);
			EditorUI::SetupFiguresColumn("Captured", k_widestValue);
			EditorUI::SetupFiguresColumn("Draws", k_widestCount);
			EditorUI::SetupFiguresColumn("Relit", k_widestValue);
			EditorUI::SetupFiguresColumn("", k_widestUnit);
			FiguresHeaderRow(k_columnCount);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Irradiance");
			FigureCell(FormatFixed(work.IrradianceProbesCaptured, 1));
			FigureCell(StringUtils::FormatCount(work.IrradianceCaptureDraws));
			FigureCell(FormatFixed(work.IrradianceProbesRelit, 0));

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Reflection");
			FigureCell(FormatFixed(work.ReflectionProbesCaptured, 2));
			FigureCell(StringUtils::FormatCount(work.ReflectionCaptureDraws));
			FigureCell(FormatFixed(work.ReflectionProbesRelit, 2));

			ImGui::EndTable();
		}
#endif

		// A table of labelled values, with their units after them. Pair with ImGui::EndTable if it returns true.
		bool BeginLabelTable(const char* id)
		{
			// The same padding as the figure tables, so the values end where the probe table's figures do
			if (!BeginFiguresTable(id, 3, ImGuiTableFlags_None))
			{
				return false;
			}
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
			EditorUI::SetupFiguresColumn("", k_widestValue);
			EditorUI::SetupFiguresColumn("", k_widestUnit);
			return true;
		}

		// label, then value against the right edge of its column, then unit; indented under the row before if isPart
		void LabelRow(const char* label, const std::string& value, const char* unit, bool isPart = false)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (isPart)
			{
				ImGui::Indent();
			}
			ImGui::TextUnformatted(label);
			if (isPart)
			{
				ImGui::Unindent();
			}

			ImGui::TableNextColumn();
			EditorUI::TextAlignedRight(value.c_str());
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", unit);
		}
	}

	namespace FrameStatsView
	{
		void DrawLine(const ImVec2& position)
		{
			const FrameStats& stats = RZE().GetFramePacer().GetStats();

			ImDrawList& drawList = *ImGui::GetWindowDrawList();
			const ImU32 colour = ImGui::GetColorU32(ImGuiCol_TextDisabled);
			const float y = position.y;
			float x = position.x;
			x = DrawFigure(drawList, x, y, colour, FormatFixed(stats.FrameTimeMS, 1), k_widestBarMS);
			x = DrawWords(drawList, x, y, colour, " ms");
			x = DrawWords(drawList, x, y, colour, k_dot);
			x = DrawFigure(drawList, x, y, colour, FormatFixed(stats.FramesPerSecond, 0), k_widestBarRate);
			x = DrawWords(drawList, x, y, colour, " FPS");
			x = DrawWords(drawList, x, y, colour, k_dot);
			x = DrawWords(drawList, x, y, colour, "idle ");
			x = DrawFigure(drawList, x, y, colour, FormatFixed(stats.IdleTimeMS, 1), k_widestBarMS);
#ifdef RZE_RENDER_STATS
			x = DrawWords(drawList, x, y, colour, " ms");
			x = DrawWords(drawList, x, y, colour, k_dot);
			x = DrawWords(drawList, x, y, colour, "render ");
			const Rendering::RenderThreadStats renderThreadStats = Rendering::Renderer::GetRenderThreadStats();
			x = DrawFigure(drawList, x, y, colour, FormatFixed(renderThreadStats.BusyTimeMS, 1), k_widestBarMS);
#endif
			DrawWords(drawList, x, y, colour, " ms");
		}

		void DrawDetails()
		{
			const FrameStats& stats = RZE().GetFramePacer().GetStats();

			ImGui::SeparatorText("Main thread");
			if (BeginLabelTable("##MainThread"))
			{
				LabelRow("Frame", FormatMS(stats.FrameTimeMS), "ms");
				LabelRow("Frames a second", FormatFixed(stats.FramesPerSecond, 1), "");
				LabelRow("UI updates a second", FormatFixed(stats.UIFramesPerSecond, 1), "");
				LabelRow("Idle", FormatMS(stats.IdleTimeMS), "ms");
				LabelRow("on tasks", FormatMS(stats.IdleWorkMS), "ms", true);
				LabelRow("waiting for the next frame", FormatMS(std::max(0.0, stats.IdleTimeMS - stats.IdleWorkMS)), "ms", true);
				ImGui::EndTable();
			}

#ifdef RZE_RENDER_STATS
			// Counted while this shows, and for a frame or so after
			RZE().GetRenderEngine().RequestGPUStatistics();
			const RenderStats& renderStats = RZE().GetRenderEngine().GetStats();
			const Rendering::RenderThreadStats renderThreadStats = Rendering::Renderer::GetRenderThreadStats();

			ImGui::SeparatorText("Rendering");
			if (BeginLabelTable("##Rendering"))
			{
				LabelRow("Draw calls, not the UI's", StringUtils::FormatCount(renderStats.DrawCalls), "");
				LabelRow("Instances", StringUtils::FormatCount(renderStats.Instances), "");
				LabelRow("Recording the stages", FormatMS(renderStats.RecordTimeMS), "ms");
				ImGui::EndTable();
			}
			ImGui::Spacing();
			DrawStageTable(renderStats);
			if (!renderStats.CanCountGPUStatistics)
			{
				ImGui::TextDisabled("The GPU's counts are off (-gpustats 0), or the graphics driver doesn't have them.");
			}

			// The scene's probe and shadow work: what objects and lights changing cost, and the probes' relighting
			const RenderWorkCounts& work = renderStats.Work;
			ImGui::Indent();
			ImGui::SeparatorText("Scene updates, a frame");
			DrawProbeTable(work);
			if (BeginLabelTable("##Shadows"))
			{
				LabelRow("Sun shadow maps", FormatFixed(work.SunShadowMapsRendered, 2), "");
				LabelRow("Point light shadow faces", FormatFixed(work.PointShadowFacesRendered, 2), "");
				LabelRow("Shadow draws", StringUtils::FormatCount(work.ShadowDraws), "");
				ImGui::EndTable();
			}
			ImGui::Unindent();

			// Part of rendering, so it sits under it
			ImGui::Indent();
			ImGui::SeparatorText("Render thread");
			if (BeginLabelTable("##RenderThread"))
			{
				LabelRow("Busy", FormatMS(renderThreadStats.BusyTimeMS), "ms");
				LabelRow("running commands", FormatMS(std::max(0.0, renderThreadStats.BusyTimeMS - renderThreadStats.PresentTimeMS)), "ms", true);
				LabelRow("presenting", FormatMS(renderThreadStats.PresentTimeMS), "ms", true);
				LabelRow("Waiting for the main thread", FormatMS(std::max(0.0, stats.FrameTimeMS - renderThreadStats.BusyTimeMS)), "ms");
				// Long presents with vsync off mean the GPU is behind: the driver holds the swap until it catches up
				LabelRow("GPU, up to presenting", FormatMS(renderThreadStats.GPUFrameTimeMS), "ms");
				ImGui::EndTable();
			}
			ImGui::Unindent();

#endif

			ImGui::Spacing();
			ImGui::TextDisabled("Averaged over %.1f s. Frames only have idle time with vsync or a frame rate cap.", StatsWindow::k_windowSeconds);
#ifdef RZE_RENDER_STATS
			ImGui::TextDisabled("The GPU counts what it draws only while this shows, which adds a little to its time.");
#endif
		}
	}
}
