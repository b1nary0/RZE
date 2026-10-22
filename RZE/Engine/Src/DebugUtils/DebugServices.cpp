#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#define MAX_LOG_SIZE 10

DebugServices::DebugServices()
{
}

void DebugServices::Trace(LogChannel channel, const std::string& text)
{
	const LogChannelIndex channelIndex = GetLogChannelIndex(channel);
	{
		// Both locks are needed: the channel lookup records the index of the entry about to be added.
		std::scoped_lock guard(mChannelLookupsMutex, mDataEntriesMutex);

		U16& channelLookup = mChannelLookups[channelIndex].emplace_back();
		channelLookup = static_cast<U16>(mDataEntries.size());

		LogEntry& entry = mDataEntries.emplace_back();

		entry.Channel = channel;
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
	const LogChannelIndex buildChannelIndex = GetLogChannelIndex(LogChannel::Build);
	const LogChannelIndex debugChannelIndex = GetLogChannelIndex(LogChannel::Debug);
	const LogChannelIndex infoChannelIndex = GetLogChannelIndex(LogChannel::Info);
	const LogChannelIndex warningChannelIndex = GetLogChannelIndex(LogChannel::Warning);
	const LogChannelIndex errorChannelIndex = GetLogChannelIndex(LogChannel::Error);

	mChannelColours[buildChannelIndex][0] = 0.75f;
	mChannelColours[buildChannelIndex][1] = 0.75f;
	mChannelColours[buildChannelIndex][2] = 0.75f;

	// Muted, since it's the high-volume detail
	mChannelColours[debugChannelIndex][0] = 0.50f;
	mChannelColours[debugChannelIndex][1] = 0.58f;
	mChannelColours[debugChannelIndex][2] = 0.70f;

	mChannelColours[infoChannelIndex][0] = 1.0f;
	mChannelColours[infoChannelIndex][1] = 1.0f;
	mChannelColours[infoChannelIndex][2] = 1.0f;

	// Warning and Error match the editor's task colours (Editor/Src/UI/TaskColours.h)
	mChannelColours[warningChannelIndex][0] = 1.0f;
	mChannelColours[warningChannelIndex][1] = 0.60f;
	mChannelColours[warningChannelIndex][2] = 0.20f;

	mChannelColours[errorChannelIndex][0] = 1.0f;
	mChannelColours[errorChannelIndex][1] = 0.42f;
	mChannelColours[errorChannelIndex][2] = 0.36f;
}
