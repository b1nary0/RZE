#include <Utils/Memory/ByteStreamUtils.h>

namespace ByteStreamUtils
{
	std::string ReadString(ByteStream& byteStream)
	{
		const size_t sizeBytes = ReadType<size_t>(byteStream);
		if (byteStream.HasFailed() || sizeBytes > byteStream.GetRemainingLength())
		{
			byteStream.Fail();
			return std::string();
		}

		std::string string(sizeBytes, '\0');
		byteStream.ReadBytes(string.data(), sizeBytes);
		return string;
	}

	size_t ReadArrayCount(ByteStream& byteStream, size_t elementSizeBytes)
	{
		const size_t sizeBytes = ReadType<size_t>(byteStream);
		if (byteStream.HasFailed() || sizeBytes > byteStream.GetRemainingLength() || sizeBytes % elementSizeBytes != 0)
		{
			byteStream.Fail();
			return 0;
		}

		return sizeBytes / elementSizeBytes;
	}
}
