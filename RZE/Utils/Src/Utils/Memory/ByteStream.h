#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/Filepath.h>

// A byte buffer read or written front to back. Reading or writing past the end doesn't touch memory beyond the
// buffer: the stream fails instead, and stays failed (HasFailed), so a reader can read every field and check once.
// A file that's too short or holds a wrong length is bad data, not a bug, so failing isn't asserted.
class ByteStream
{
public:
	ByteStream(const std::string& name);
	ByteStream(const std::string& name, size_t streamLength);
	~ByteStream();

	// It owns its buffer
	ByteStream(const ByteStream&) = delete;
	ByteStream& operator=(const ByteStream&) = delete;

public:
	inline const Byte* GetBytes() const { return mBytes; }

	// Returns false (leaving the stream empty) if the file can't be opened.
	bool ReadFromFile(const Filepath& filePath);

	size_t GetLength() const { return mStreamLength; }
	// Bytes between the cursor and the end
	size_t GetRemainingLength() const { return mStreamLength - mCurPos; }
	// Whether a read or write has tried to go past the end, or a reader called Fail
	bool HasFailed() const { return mHasFailed; }
	// For a reader that finds the data can't be right, e.g. a length longer than what's left
	void Fail() { mHasFailed = true; }

	// #TODO
	// WriteToStream/FlushToFile type functions to flesh out this class.
	// Then the File class can essentially just hold one of these

	// The bytes at the cursor, which may not be aligned for any type: copy out of them (ReadBytes), don't cast them.
	// Null at the end of the stream.
	Byte* PeekBytes();
	// PeekBytes, then moves the cursor on sizeBytes. If fewer than sizeBytes remain, fails and gives null.
	Byte* PeekBytesAdvance(size_t sizeBytes);

	// Copies sizeBytes into buf. If fewer remain, zero-fills buf, fails and returns false.
	bool ReadBytes(void* buf, size_t sizeBytes);
	// Copies sizeBytes from buf. If they don't fit, writes nothing, fails and returns false.
	bool WriteBytes(const void* buf, size_t sizeBytes);

	size_t GetNumBytesWritten() const { return mCurPos; }

private:
	std::string mName;
	Byte* mBytes = nullptr;
	size_t mCurPos = 0; // Byte "cursor" - index of byte next to be read
	size_t mStreamLength = 0;
	bool mHasFailed = false;
};
