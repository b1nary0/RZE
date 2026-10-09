#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/Filepath.h>

class ByteStream
{
public:
	ByteStream(const std::string& name);
	ByteStream(const std::string& name, size_t streamLength);
	~ByteStream();

public:
	inline const Byte* GetBytes() const { return mBytes; }

	// Returns false (leaving the stream empty) if the file can't be opened.
	bool ReadFromFile(const Filepath& filePath);

	size_t GetLength() const { return mStreamLength; }

	// #TODO
	// WriteToStream/FlushToFile type functions to flesh out this class.
	// Then the File class can essentially just hold one of these 

	Byte* PeekBytes(); // For when you just want to take a look at the bytes, like reinterpret_cast to get at data
	Byte* PeekBytesAdvance(size_t sizeBytes); // PeekBytes and advance cursor position

	bool ReadBytes(Byte* buf, size_t sizeBytes); // Will copy sizeBytes into client allocated buffer
	bool WriteBytes(const void* buf, size_t sizeBytes);

	size_t GetNumBytesWritten() const { return mCurPos; }

private:
	std::string mName;
	Byte* mBytes = nullptr;
	size_t mCurPos = 0; // Byte "cursor" - index of byte next to be read
	size_t mStreamLength = 0;
};