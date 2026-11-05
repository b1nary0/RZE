#include <HarnessChannel.h>

#include <HarnessStdin.h>

#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

namespace EditorHarness
{
	// What the thread reading stdin hands the main thread. The thread is blocked reading until the driver closes
	// stdin, so it can't be joined: it's detached, and keeps this alive itself. It reads with ReadStandardInput, not
	// std::cin, so it holds no lock the process needs to exit.
	struct HarnessChannel::Inbox
	{
		std::mutex Mutex;
		std::deque<std::string> Lines;
	};

	HarnessChannel::HarnessChannel()
		: m_inbox(std::make_shared<Inbox>())
	{
		std::thread([inbox = m_inbox]()
			{
				std::string pending;
				char buffer[4096];
				size_t count = 0;
				bool firstLine = true;
				while ((count = ReadStandardInput(buffer, sizeof(buffer))) > 0)
				{
					pending.append(buffer, count);
					size_t end = 0;
					while ((end = pending.find('\n')) != std::string::npos)
					{
						std::string line = pending.substr(0, end);
						pending.erase(0, end + 1);
						// A UTF-8 byte order mark before the first command, which Windows PowerShell 5.1's
						// Process.Start writes to a redirected stdin
						static const std::string k_utf8BOM = "\xEF\xBB\xBF";
						if (firstLine && line.compare(0, k_utf8BOM.size(), k_utf8BOM) == 0)
						{
							line.erase(0, k_utf8BOM.size());
						}
						firstLine = false;
						// A Windows console ends its lines with \r\n
						if (!line.empty() && line.back() == '\r')
						{
							line.pop_back();
						}
						if (!line.empty())
						{
							std::lock_guard<std::mutex> lock(inbox->Mutex);
							inbox->Lines.push_back(std::move(line));
						}
					}
				}
			}).detach();
	}

	HarnessChannel::~HarnessChannel() = default;

	std::vector<std::string> HarnessChannel::TakeCommands()
	{
		std::lock_guard<std::mutex> lock(m_inbox->Mutex);
		std::vector<std::string> lines(std::make_move_iterator(m_inbox->Lines.begin()), std::make_move_iterator(m_inbox->Lines.end()));
		m_inbox->Lines.clear();
		return lines;
	}

	void HarnessChannel::Reply(const std::string& json)
	{
		// One write, so a log line from another thread can't land in the middle of it
		const std::string line = k_replyPrefix + json + "\n";
		std::fputs(line.c_str(), stdout);
		std::fflush(stdout);
	}
}
