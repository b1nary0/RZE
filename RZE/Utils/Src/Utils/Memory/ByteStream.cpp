#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>

#include <cstring>
#include <fstream>

ByteStream::ByteStream(const std::string& name)
	: mName(name)
{
}

ByteStream::ByteStream(const std::string& name, size_t streamLength)
	: mName(name)
	, mStreamLength(streamLength)
{
	mBytes = new Byte[streamLength];
	memset(mBytes, 0, streamLength);
}

ByteStream::~ByteStream()
{
	if (mBytes != nullptr)
	{
		delete[] mBytes;
		mBytes = nullptr;
	}
}

bool ByteStream::ReadFromFile(const Filepath& filePath)
{
	if (mBytes != nullptr)
	{
		delete[] mBytes;
		mBytes = nullptr;
	}

	mStreamLength = 0;
	mCurPos = 0;
	mHasFailed = false;

	std::ifstream input(filePath.GetAbsolutePath().c_str(), std::fstream::binary);
	if (!input.is_open())
	{
		return false;
	}

	input.seekg(0, input.end);
	const std::streamoff length = input.tellg();
	input.seekg(0, input.beg);

	if (length <= 0)
	{
		return false;
	}

	mStreamLength = static_cast<size_t>(length);

	mBytes = new unsigned char[mStreamLength];
	input.read((char*)mBytes, mStreamLength);

	input.close();
	AssertExpr(!input.is_open());

	return true;
}

Byte* ByteStream::PeekBytes()
{
	return mCurPos < mStreamLength ? &mBytes[mCurPos] : nullptr;
}

Byte* ByteStream::PeekBytesAdvance(size_t sizeBytes)
{
	if (sizeBytes > GetRemainingLength())
	{
		mHasFailed = true;
		return nullptr;
	}

	Byte* const bytes = PeekBytes();
	mCurPos += sizeBytes;
	return bytes;
}

bool ByteStream::ReadBytes(void* buf, size_t sizeBytes)
{
	// Nothing to copy, and the buffer may not exist (memcpy needs a pointer even for 0 bytes)
	if (sizeBytes == 0)
	{
		return !mHasFailed;
	}

	if (sizeBytes > GetRemainingLength())
	{
		// Zeroed, so a reader that carries on before checking HasFailed reads zeros rather than garbage
		memset(buf, 0, sizeBytes);
		mHasFailed = true;
		return false;
	}

	memcpy(buf, &mBytes[mCurPos], sizeBytes);
	mCurPos += sizeBytes;

	return true;
}

bool ByteStream::WriteBytes(const void* buf, size_t sizeBytes)
{
	if (sizeBytes == 0)
	{
		return !mHasFailed;
	}

	if (sizeBytes > GetRemainingLength())
	{
		mHasFailed = true;
		return false;
	}

	memcpy(&mBytes[mCurPos], buf, sizeBytes);
	mCurPos += sizeBytes;

	return true;
}
