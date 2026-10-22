#pragma once

#include <deque>
#include <string>
#include <vector>
#include <mutex>
#include <type_traits>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector3D.h>

// Info, Debug, Warning and Error are the RZE_LOG levels (Debug::ELogLevel); Build is the editor's build and launch output
enum class LogChannel
{
	Info,
	Debug,
	Build,
	Warning,
	Error,

	Count
};

using LogChannelIndex = std::underlying_type_t<LogChannel>;

// A channel as its underlying value, to index per-channel arrays (sized by LogChannel::Count)
constexpr LogChannelIndex GetLogChannelIndex(LogChannel channel)
{
	return static_cast<LogChannelIndex>(channel);
}

class DebugServices
{
public:
	struct LogEntry
	{
		Vector3D TextColor;
		std::string Text;
	};

public:
	DebugServices();

	static DebugServices& Get()
	{
		static DebugServices instance;
		return instance;
	}

	void Initialize();
	void Trace(LogChannel channel, const std::string& text);

public:
	// Main thread only. The returned reference is not protected once this returns, so worker threads
	// must not Trace() while it is being iterated; marshal log calls via Threading::MainThreadDispatcher instead.
	const std::vector<LogEntry>& GetLogEntries();

private:
	// #NOTE
	// Index into mChannelLookup is LogChannel enum. std::vector represents which entry in mDataEntries
	// is posted to this channel.
	std::vector<U16> mChannelLookups[GetLogChannelIndex(LogChannel::Count)];
	std::vector<LogEntry> mDataEntries;

	std::mutex mEntryAddMutex;
	std::mutex mDataEntriesMutex;
	std::mutex mChannelLookupsMutex;

	// #NOTE
	// If LogChannel::Build is tracing, mChannelColours[Build][0] is the r component of the trace colour.
	float mChannelColours[GetLogChannelIndex(LogChannel::Count)][3];
};