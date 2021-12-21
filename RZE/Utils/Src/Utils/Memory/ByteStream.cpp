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
	mBytes = new unsigned char[streamLength];
	memset(mBytes, NULL, streamLength);
}

ByteStream::~ByteStream()
{
	if (mBytes != nullptr)
	{
		LOG_CONSOLE_ARGS("ByteStream [%s] destroying with live buffer", mName.c_str());

		delete[] mBytes;
		mBytes = nullptr;
	}
}

void ByteStream::ReadFromFile(const FilePath& filePath)
{
	if (mBytes != nullptr)
	{
		delete[] mBytes;
		mBytes = nullptr;
	}

	std::ifstream input(filePath.GetAbsolutePath().c_str(), std::fstream::binary);
	input.seekg(0, input.end);
	size_t length = input.tellg();
	input.seekg(0, input.beg);

	mStreamLength = length;

	mBytes = new unsigned char[length];
	input.read((char*)mBytes, length);

	input.close();
	AssertExpr(!input.is_open());
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

