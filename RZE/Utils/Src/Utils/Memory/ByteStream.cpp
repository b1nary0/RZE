#include <StdAfx.h>
#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>

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
	memset(mBytes, NULL, streamLength);
}

ByteStream::~ByteStream()
{
	if (mBytes != nullptr)
	{
		// #TODO
		// implement logging verbosity and then re-enable this.
		//RZE_LOG_ARGS("ByteStream [%s] destroying with live buffer", mName.c_str());

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
	AssertMsg(mCurPos < mStreamLength, "Attempting to peek past the stream length");
	return &mBytes[mCurPos];
}

Byte* ByteStream::PeekBytesAdvance(size_t sizeBytes)
{
	AssertMsg(mCurPos + sizeBytes <= mStreamLength, 
		"Attempting to advance stream cursor past stream length");

	Byte* bytes = PeekBytes();
	mCurPos += sizeBytes;
	return bytes;
}

bool ByteStream::ReadBytes(Byte* buf, size_t sizeBytes)
{
	AssertMsg(mCurPos + sizeBytes <= mStreamLength,
		"Attempting to read past stream length");

	memcpy(buf, &mBytes[mCurPos], sizeBytes);
	mCurPos += sizeBytes;

	return true;
}

bool ByteStream::WriteBytes(const void* buf, size_t sizeBytes)
{
	AssertMsg(mCurPos + sizeBytes <= mStreamLength,
		"Attempting to write past stream length");

	memcpy(&mBytes[mCurPos], buf, sizeBytes);
	mCurPos += sizeBytes;

	return true;
}

