#pragma once

#include <deque>
#include <string>
#include <vector>

#include <Utils/Math/Vector3D.h>

enum class LogChannel
{
	Info,
	Debug,
	Build
};
constexpr int ChannelCount = 3;

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
	void AddData(const std::string& text, const Vector3D& color = Vector3D(1.0f, 1.0f, 0.0f));
	void Trace(LogChannel channel, const std::string& text);

public:
	const std::vector<LogEntry>& GetLogEntries();

private:
	// #NOTE
	// Index into mChannelLookup is LogChannel enum. std::vector represents which entry in mDataEntries
	// is posted to this channel.
	std::vector<U16> mChannelLookups[ChannelCount];
	std::vector<LogEntry> mDataEntries;

	std::mutex mEntryAddMutex;
	std::mutex mDataEntriesMutex;
	std::mutex mChannelLookupsMutex;

	// #NOTE
	// If LogChannel::Build is tracing, mChannelColours[Build][0] is the r component of the trace colour.
	float mChannelColours[ChannelCount][3];
};