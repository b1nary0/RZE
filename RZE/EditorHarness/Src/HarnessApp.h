#pragma once

#include <EditorApp.h>
#include <HarnessChannel.h>
#include <HarnessInput.h>

#include <Utils/PrimitiveDefs.h>

#include <RapidJSON/document.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Rendering
{
	struct TextureReadback;
}

namespace EditorHarness
{
	// The Editor, run by a driver through HarnessChannel. Each frame's commands run on the main thread before the UI
	// is built, so they act on the frame as a user's input would. Input commands answer once they've played out.
	class HarnessApp : public Editor::EditorApp
	{
	public:
		void PreUIUpdate() override;

	private:
		// Answers line, now or once its input has played out
		void RunCommand(const std::string& line);
		// False, with outError saying why, if the command's input can't be made
		bool PlayInput(const std::string& name, const rapidjson::Value& command, int64_t id, std::string& outError);
		// Writes and answers the screenshots whose frames have been read back
		void FinishShots();

	private:
		// A screenshot waiting for its frame to be read back
		struct PendingShot
		{
			int64_t ID = -1;
			std::string Path;
			std::shared_ptr<Rendering::TextureReadback> Readback;
		};

		HarnessChannel m_channel;
		std::vector<PendingShot> m_pendingShots;
		HarnessInput m_input;
		U64 m_frameCount = 0;
	};
}
