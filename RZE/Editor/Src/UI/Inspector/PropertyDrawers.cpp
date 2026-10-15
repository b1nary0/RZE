#include <UI/Inspector/PropertyDrawers.h>

#include <Services/FolderListing.h>
#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace
{
	using Editor::PropertyDrawers::Drawer;

	constexpr char k_defaultFloatFormat[] = "%.3f";
	// Per pixel, for drags without a step
	constexpr float k_defaultDragSpeed = 0.01f;
	constexpr size_t k_maxStringLength = 256;
	// How far a popup shown on hover (a thumbnail's preview, a path) sits from the cursor, so the cursor doesn't cover it
	constexpr float k_hoverPopupCursorOffset = 8.0f;

	const char* GetFormat(const PropertyDescriptor& property)
	{
		return property.Format.empty() ? k_defaultFloatFormat : property.Format.c_str();
	}

	float Clamp(const PropertyDescriptor& property, float value)
	{
		return property.HasRange() ? MathUtils::Clampf(value, property.Min, property.Max) : value;
	}

	// Typed text commits on Enter, so a half-typed value (e.g. a scale of 0 on the way to 0.5) is never applied.
	// Drags and sliders apply as they move.
	constexpr ImGuiInputTextFlags k_commitOnEnter = ImGuiInputTextFlags_EnterReturnsTrue;

	bool DrawBool(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		return ImGui::Checkbox("##value", &std::get<bool>(value));
	}

	bool DrawInt(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		int& intValue = std::get<int>(value);
		const int min = static_cast<int>(property.Min);
		const int max = static_cast<int>(property.Max);

		bool changed = false;
		if (property.Widget == EPropertyWidget::Slider && property.HasRange())
		{
			changed = ImGui::SliderInt("##value", &intValue, min, max);
		}
		else if (property.Widget == EPropertyWidget::Drag)
		{
			changed = ImGui::DragInt("##value", &intValue, property.Step > 0.0f ? property.Step : 1.0f, min, max);
		}
		else
		{
			const int step = std::max(1, static_cast<int>(property.Step));
			changed = ImGui::InputInt("##value", &intValue, step, step * 10, k_commitOnEnter);
		}

		if (changed && property.HasRange())
		{
			intValue = std::clamp(intValue, min, max);
		}
		return changed;
	}

	bool DrawFloat(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		float& floatValue = std::get<float>(value);

		bool changed = false;
		if (property.Widget == EPropertyWidget::Slider && property.HasRange())
		{
			changed = ImGui::SliderFloat("##value", &floatValue, property.Min, property.Max, GetFormat(property));
		}
		else if (property.Widget == EPropertyWidget::Drag)
		{
			const float speed = property.Step > 0.0f ? property.Step : k_defaultDragSpeed;
			changed = ImGui::DragFloat("##value", &floatValue, speed, property.Min, property.Max, GetFormat(property));
		}
		else
		{
			changed = ImGui::InputFloat("##value", &floatValue, property.Step, property.Step * 10.0f, GetFormat(property), k_commitOnEnter);
		}

		if (changed)
		{
			floatValue = Clamp(property, floatValue);
		}
		return changed;
	}

	bool DrawFloats(const PropertyDescriptor& property, float* values, int count)
	{
		bool changed = false;
		if (property.Widget == EPropertyWidget::Drag)
		{
			const float speed = property.Step > 0.0f ? property.Step : k_defaultDragSpeed;
			changed = ImGui::DragScalarN("##value", ImGuiDataType_Float, values, count, speed,
				property.HasRange() ? &property.Min : nullptr, property.HasRange() ? &property.Max : nullptr, GetFormat(property));
		}
		else
		{
			changed = ImGui::InputScalarN("##value", ImGuiDataType_Float, values, count, nullptr, nullptr, GetFormat(property), k_commitOnEnter);
		}

		if (changed)
		{
			for (int i = 0; i < count; ++i)
			{
				values[i] = Clamp(property, values[i]);
			}
		}
		return changed;
	}

	bool DrawVector3(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		Vector3D& vector = std::get<Vector3D>(value);
		if (property.Widget == EPropertyWidget::Colour)
		{
			return ImGui::ColorEdit3("##value", vector.GetValuePtr(), ImGuiColorEditFlags_Float);
		}
		return DrawFloats(property, vector.GetValuePtr(), 3);
	}

	bool DrawVector4(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		Vector4D& vector = std::get<Vector4D>(value);
		if (property.Widget == EPropertyWidget::Colour)
		{
			return ImGui::ColorEdit4("##value", vector.GetValuePtr(), ImGuiColorEditFlags_Float);
		}
		return DrawFloats(property, vector.GetValuePtr(), 4);
	}

	bool DrawString(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		std::string& stringValue = std::get<std::string>(value);
		if (isReadOnly)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(stringValue.c_str());
			return false;
		}

		char buffer[k_maxStringLength];
		strncpy_s(buffer, stringValue.c_str(), _TRUNCATE);
		if (ImGui::InputText("##value", buffer, sizeof(buffer), k_commitOnEnter))
		{
			stringValue = buffer;
			return true;
		}
		return false;
	}

	// Shows text as a tooltip just above the cursor, centred on it and kept inside the viewport, so it doesn't cover the
	// rows below. Setting the position stops ImGui keeping it on screen, so that's done here with the size it will have.
	void SetTooltipAboveCursor(const char* text)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 size(textSize.x + style.WindowPadding.x * 2.0f, textSize.y + style.WindowPadding.y * 2.0f);

		const ImGuiViewport* const viewport = ImGui::GetWindowViewport();
		const ImVec2 mouse = ImGui::GetMousePos();
		const float right = viewport->WorkPos.x + viewport->WorkSize.x;
		const float x = std::max(viewport->WorkPos.x, std::min(mouse.x - size.x * 0.5f, right - size.x));
		const float y = std::max(viewport->WorkPos.y, mouse.y - k_hoverPopupCursorOffset - size.y);
		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
		ImGui::SetTooltip("%s", text);
	}

	// A preview of the asset (if the property has one), its file name (its relative path shows above the cursor on
	// hover), a button that picks a new one in the file browser, and one that clears it (if the property allows it).
	// Fits the item width it's given, so the inspector can leave room beside it.
	bool DrawAssetPath(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		Filepath& path = std::get<Filepath>(value);
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		float width = ImGui::CalcItemWidth();

		if (property.GetThumbnail)
		{
			const float thumbnailSize = ImGui::GetFrameHeight();
			Editor::PropertyDrawers::DrawThumbnail(property.GetThumbnail(object), thumbnailSize);
			ImGui::SameLine(0.0f, spacing);
			width -= thumbnailSize + spacing;
		}

		std::string displayName = path.IsValid() ? path.GetFileName() : "(none)";
		if (isReadOnly)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(displayName.c_str());
			if (ImGui::IsItemHovered() && path.IsValid())
			{
				SetTooltipAboveCursor(path.GetRelativePath().c_str());
			}
			return false;
		}

		const char* const browseLabel = "...";
		const float browseWidth = ImGui::CalcTextSize(browseLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const bool canClear = property.IsClearable && path.IsValid();
		width -= browseWidth + spacing;
		if (canClear)
		{
			width -= Editor::EditorUI::GetIconButtonWidth() + spacing;
		}

		ImGui::SetNextItemWidth(std::max(1.0f, width));
		ImGui::InputText("##path", displayName.data(), displayName.size() + 1, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::IsItemHovered() && path.IsValid())
		{
			SetTooltipAboveCursor(path.GetRelativePath().c_str());
		}

		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		Editor::FileBrowserModal& browser = context.GetFileBrowser();
		const ImGuiID browseID = ImGui::GetID("##browse");
		if (ImGui::Button(browseLabel))
		{
			Editor::FileBrowserRequest request;
			request.Title = "Select " + property.Name;
			request.ConfirmLabel = "Select";
			request.RootFolder = Filepath(property.AssetDirectory);
			request.Extensions = Editor::ParseExtensionFilter(property.AssetFilter);
			request.InitialSelection = path;
			request.ThumbnailFor = &Editor::Thumbnails::FindFor;
			// Mesh thumbnails, for one; does nothing for files without rendered thumbnails
			context.GetThumbnails().AddToBrowser(request);
			request.RequesterID = browseID;
			browser.Open(std::move(request));
		}

		if (canClear)
		{
			ImGui::SameLine(0.0f, spacing);
			if (Editor::EditorUI::IconButton("##clear", Editor::EditorUI::EIcon::Clear, "Clear"))
			{
				path = Filepath();
				return true;
			}
		}

		// The pick comes back on a later frame, once the browser closes
		return browser.ConsumeResult(browseID, path);
	}

	// A drop-down of the property's EnumNames
	bool DrawEnum(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		int& index = std::get<int>(value);
		const bool isKnown = index >= 0 && index < static_cast<int>(property.EnumNames.size());

		bool changed = false;
		if (ImGui::BeginCombo("##value", isKnown ? property.EnumNames[index].c_str() : "(unknown)"))
		{
			for (int option = 0; option < static_cast<int>(property.EnumNames.size()); ++option)
			{
				const bool isSelected = option == index;
				if (ImGui::Selectable(property.EnumNames[option].c_str(), isSelected) && !isSelected)
				{
					index = option;
					changed = true;
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	bool DrawTextureProperty(Editor::EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly)
	{
		Editor::PropertyDrawers::DrawTexture(std::get<PropertyTexture>(value));
		return false;
	}

	std::unordered_map<EPropertyType, Drawer>& GetDrawers()
	{
		static std::unordered_map<EPropertyType, Drawer> s_drawers =
		{
			{ EPropertyType::Bool, &DrawBool },
			{ EPropertyType::Int, &DrawInt },
			{ EPropertyType::Float, &DrawFloat },
			{ EPropertyType::Vector3, &DrawVector3 },
			{ EPropertyType::Vector4, &DrawVector4 },
			{ EPropertyType::String, &DrawString },
			{ EPropertyType::AssetPath, &DrawAssetPath },
			{ EPropertyType::Texture, &DrawTextureProperty },
			{ EPropertyType::Enum, &DrawEnum },
		};
		return s_drawers;
	}
}

namespace Editor
{
	namespace PropertyDrawers
	{
		void Register(EPropertyType type, Drawer drawer)
		{
			GetDrawers()[type] = drawer;
		}

		Drawer Find(EPropertyType type)
		{
			const auto it = GetDrawers().find(type);
			return it != GetDrawers().end() ? it->second : nullptr;
		}

		void DrawThumbnail(const PropertyTexture& texture, float size)
		{
			const bool hasImage = texture.Handle.IsValid() && texture.Size.X() > 0.0f && texture.Size.Y() > 0.0f;
			if (!hasImage)
			{
				// An empty frame keeps the place, so rows line up whether or not they have an image
				const ImVec2 start = ImGui::GetCursorScreenPos();
				ImGui::Dummy(ImVec2(size, size));
				ImGui::GetWindowDrawList()->AddRect(start, ImVec2(start.x + size, start.y + size), ImGui::GetColorU32(ImGuiCol_Border),
					ImGui::GetStyle().FrameRounding);
				return;
			}

			// Square, whatever the image's shape, so thumbnails line up
			const ImVec2 start = ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(size, size));
			const bool isHovered = ImGui::IsItemHovered();
			const float scale = size / std::max(texture.Size.X(), texture.Size.Y());
			const ImVec2 imageSize(texture.Size.X() * scale, texture.Size.Y() * scale);
			const ImVec2 imageStart(start.x + (size - imageSize.x) * 0.5f, start.y + (size - imageSize.y) * 0.5f);
			ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)texture.Handle.GetTextureData(), imageStart,
				ImVec2(imageStart.x + imageSize.x, imageStart.y + imageSize.y), ImVec2(0.0f, 0.0f), ImVec2(texture.UVMax.X(), texture.UVMax.Y()));

			if (isHovered)
			{
				// Below and to the left of the cursor, the side away from the inspector's other rows and the scene,
				// moved up if it would run off the bottom. Setting the position stops ImGui keeping it on screen, so
				// that's done here with the size it will have.
				const ImGuiStyle& style = ImGui::GetStyle();
				const float previewScale = std::min(1.0f, k_thumbnailPreviewSize / std::max(texture.Size.X(), texture.Size.Y()));
				const float previewHeight = texture.Size.Y() * previewScale + style.ItemSpacing.y + ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0f;
				const ImGuiViewport* const viewport = ImGui::GetWindowViewport();
				const ImVec2 mouse = ImGui::GetMousePos();
				const float bottom = viewport->WorkPos.y + viewport->WorkSize.y;
				const float y = std::max(viewport->WorkPos.y, std::min(mouse.y + k_hoverPopupCursorOffset, bottom - previewHeight));
				ImGui::SetNextWindowPos(ImVec2(mouse.x - k_hoverPopupCursorOffset, y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
			}
			if (isHovered && ImGui::BeginTooltip())
			{
				DrawTexture(texture, k_thumbnailPreviewSize);
				ImGui::TextDisabled("%d x %d", static_cast<int>(texture.Size.X()), static_cast<int>(texture.Size.Y()));
				ImGui::EndTooltip();
			}
		}

		void DrawTexture(const PropertyTexture& texture, float maxSize)
		{
			const Rendering::TextureBuffer2DHandle& handle = texture.Handle;
			if (!handle.IsValid() || texture.Size.X() <= 0.0f || texture.Size.Y() <= 0.0f)
			{
				ImGui::TextDisabled("(no image)");
				return;
			}

			const float scale = maxSize > 0.0f ? std::min(1.0f, maxSize / std::max(texture.Size.X(), texture.Size.Y())) : 1.0f;
			ImGui::Image((ImTextureID)(intptr_t)handle.GetTextureData(), ImVec2(texture.Size.X() * scale, texture.Size.Y() * scale),
				ImVec2(0.0f, 0.0f), ImVec2(texture.UVMax.X(), texture.UVMax.Y()));
		}
	}
}
