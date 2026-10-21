#include <Rendering/UIFrameData.h>

#include <imGUI/imgui.h>

namespace Rendering
{
	namespace
	{
		ImDrawData* CloneDrawData(ImDrawData& toClone)
		{
			ImDrawData* clone = new ImDrawData();
			clone->Valid = toClone.Valid;
			clone->CmdListsCount = toClone.CmdListsCount;
			clone->TotalIdxCount = toClone.TotalIdxCount;
			clone->TotalVtxCount = toClone.TotalVtxCount;
			clone->DisplayPos = toClone.DisplayPos;
			clone->DisplaySize = toClone.DisplaySize;
			clone->FramebufferScale = toClone.FramebufferScale;

			clone->CmdLists = toClone.CmdLists;
			for (int i = 0; i < toClone.CmdListsCount; ++i)
			{
				ImDrawList* cmdList = toClone.CmdLists[i]->CloneOutput();
				// Resolve texture refs to plain IDs so the render thread never reads the main thread's ImTextureData
				for (ImDrawCmd& drawCmd : cmdList->CmdBuffer)
				{
					drawCmd.TexRef = ImTextureRef(drawCmd.GetTexID());
				}
				clone->CmdLists[i] = cmdList;
			}

			clone->OwnerViewport = toClone.OwnerViewport;
			// Texture requests are handled on the main thread in Renderer::ImGuiRender(), so the backend must skip them
			clone->Textures = nullptr;

			return clone;
		}
	}

	UIFrameData::UIFrameData(ImDrawData& drawData, std::vector<TextureBuffer2DHandle>&& textures)
		: m_drawData(CloneDrawData(drawData))
		, m_textures(std::move(textures))
	{
	}

	UIFrameData::~UIFrameData()
	{
		for (int i = 0; i < m_drawData->CmdListsCount; ++i)
		{
			IM_DELETE<ImDrawList>(m_drawData->CmdLists[i]);
		}
		delete m_drawData;
	}
}
