#pragma once

#include <string>

namespace ByteStreamUtils
{
	inline std::string ReadString(ByteStream& byteStream)
	{
		size_t nameSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

		unsigned char* name = new unsigned char[nameSizeBytes + 1];
		byteStream.ReadBytes(name, nameSizeBytes);
		name[nameSizeBytes] = '\0';
		std::string string((char*)name);

		delete[] name;
		name = nullptr;
		return std::move(string);
	}

	// #NOTE
	// ReadArray will only work on POD types. How could you know the length of a dynamically allocated array
	// like a string? If you can't rely on sizeof(TDataType) then this function will fail.
	template<typename TDataType>
	inline std::vector<TDataType> ReadArray(ByteStream& byteStream)
	{
		size_t dataSizeBytes = *reinterpret_cast<size_t*>(byteStream.PeekBytesAdvance(sizeof(size_t)));

		std::vector<TDataType> dataVec;
		dataVec.resize(dataSizeBytes / sizeof(TDataType));
		byteStream.ReadBytes((Byte*)dataVec.data(), dataSizeBytes);

		return dataVec;
	}

	// #NOTE
	// ReadType will only work on POD types. How could you know the length of a dynamically allocated array
	// like a string? If you can't rely on sizeof(TType) then this function will fail.
	template<typename TType>
	inline TType ReadType(ByteStream& byteStream)
	{
		TType* data =
			reinterpret_cast<TType*>(byteStream.PeekBytesAdvance(sizeof(TType)));
		return *data;
	}
}