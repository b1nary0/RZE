#pragma once

#include <memory>
#include <string>
#include <vector>

namespace EditorHarness
{
	// How a driver talks to the harness: one JSON command per line on stdin, and one answer per line on stdout,
	// prefixed with k_replyPrefix so it stands out from the log lines around it.
	class HarnessChannel
	{
	public:
		static constexpr const char* k_replyPrefix = "RZEHARNESS ";

		HarnessChannel();
		~HarnessChannel();

		HarnessChannel(const HarnessChannel&) = delete;
		HarnessChannel& operator=(const HarnessChannel&) = delete;

		// The lines that have arrived since the last call, in order. Main thread.
		std::vector<std::string> TakeCommands();
		// Writes one answer line. Main thread.
		void Reply(const std::string& json);

	private:
		struct Inbox;
		std::shared_ptr<Inbox> m_inbox;
	};
}
