#include <Utils/Memory/ByteStream.h>

#include <Utils/DebugUtils/Debug.h>

#include <fstream>

ByteStream::ByteStream(const std::string& name)
	: mName(name)
{
}

ByteStream::ByteStream(const std::string& name, size_t streamLength)
	: mName(name)
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

	mBytes = new unsigned char[length];
	input.read((char*)mBytes, length);

	input.close();
	AssertExpr(!input.is_open());
}

bool ByteStream::WriteBytes(unsigned char* buf, size_t writeLength)
{
	memcpy(&mBytes[curPos], buf, writeLength);
	curPos += writeLength;
	return true;
}

