#pragma once

#include <Rendering/BufferHandle.h>

#include <vector>

struct ImDrawData;

namespace Rendering
{
	// A copy of one UI frame's ImGui draw data for the render thread, which draws it a frame later and again on frames
	// without a UI update. ImGui only knows a texture by its view's raw pointer, so the textures the frame draws are kept
	// here too, alive for as long as anything can still draw the copy.
	class UIFrameData
	{
	public:
		UIFrameData(ImDrawData& drawData, std::vector<TextureBuffer2DHandle>&& textures);
		~UIFrameData();

		UIFrameData(const UIFrameData&) = delete;
		UIFrameData& operator=(const UIFrameData&) = delete;

	public:
		ImDrawData* GetDrawData() const { return m_drawData; }

	private:
		ImDrawData* m_drawData;
		std::vector<TextureBuffer2DHandle> m_textures;
	};
}
