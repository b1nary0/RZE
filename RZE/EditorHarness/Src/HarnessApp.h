#pragma once

#include <EditorApp.h>
#include <HarnessChannel.h>

#include <Utils/PrimitiveDefs.h>

#include <string>

namespace EditorHarness
{
	// The Editor, run by a driver through HarnessChannel. Each frame's commands run on the main thread before the UI
	// is built, so they act on the frame as a user's input would.
	class HarnessApp : public Editor::EditorApp
	{
	public:
		void PreUIUpdate() override;

	private:
		// The answer to one command line, as JSON
		std::string RunCommand(const std::string& line);

	private:
		HarnessChannel m_channel;
		U64 m_frameCount = 0;
	};
}
