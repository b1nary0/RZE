#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#define MAX_LOG_SIZE 10

U16 GetLogChannelIndex(LogChannel channel)
{
	return std::underlying_type<LogChannel>::type(channel);
}

DebugServices::DebugServices()
{
}

void DebugServices::AddData(const std::string& text, const Vector3D& color)
{
	mDataEntries.emplace_back();
	LogEntry& entry = mDataEntries.back();

	entry.Text = text;
	entry.TextColor = color;
}

void DebugServices::Trace(LogChannel channel, const std::string& text)
{
	size_t channelIndex = std::underlying_type<LogChannel>::type(channel);
	{
		std::lock_guard<std::mutex> guard(mChannelLookupsMutex);

		mChannelLookups[channelIndex].emplace_back();
		mChannelLookups[channelIndex].back() = mDataEntries.size();
	}
	{
		std::lock_guard<std::mutex> guard(mDataEntriesMutex);

		mDataEntries.emplace_back();
		LogEntry& entry = mDataEntries.back();

		entry.Text = text;

		entry.TextColor = Vector3D(
			mChannelColours[channelIndex][0],
			mChannelColours[channelIndex][1],
			mChannelColours[channelIndex][2]);
	}
}

const std::vector<DebugServices::LogEntry>& DebugServices::GetLogEntries()
{
	std::lock_guard<std::mutex> guard(mDataEntriesMutex);
	return mDataEntries;
}

void DebugServices::Initialize()
{
	U16 buildChannelIndex = GetLogChannelIndex(LogChannel::Build);
	U16 debugChannelIndex = GetLogChannelIndex(LogChannel::Debug);
	U16 infoChannelIndex = GetLogChannelIndex(LogChannel::Info);

	mChannelColours[buildChannelIndex][0] = 0.75f;
	mChannelColours[buildChannelIndex][1] = 0.75f;
	mChannelColours[buildChannelIndex][2] = 0.75f;

	mChannelColours[debugChannelIndex][0] = 255.0f;
	mChannelColours[debugChannelIndex][1] = 255.0f;
	mChannelColours[debugChannelIndex][2] = 1.0f;

	mChannelColours[infoChannelIndex][2] = 1.0f;
	mChannelColours[infoChannelIndex][2] = 1.0f;
	mChannelColours[infoChannelIndex][2] = 1.0f;
}
