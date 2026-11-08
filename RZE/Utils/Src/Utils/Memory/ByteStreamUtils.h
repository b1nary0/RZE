#pragma once

#include <Utils/Memory/ByteStream.h>

#include <string>
#include <type_traits>
#include <vector>

// Reads what the asset writers write: a value as its bytes, and a string or an array as its size in bytes (a size_t)
// followed by the bytes. When the data runs out, or a size can't be right, they fail the stream
// (ByteStream::HasFailed) and give a zeroed or empty value, without allocating what the size claims.
namespace ByteStreamUtils
{
	std::string ReadString(ByteStream& byteStream);

	// For ReadArray: reads an array's size in bytes and gives how many elements it holds. 0, failing the stream, if
	// it's more than what's left or not a whole number of elements.
	size_t ReadArrayCount(ByteStream& byteStream, size_t elementSizeBytes);

	// Copied out of the stream, so the bytes needn't be aligned for TType
	template<typename TType>
	TType ReadType(ByteStream& byteStream)
	{
		static_assert(std::is_trivially_copyable_v<TType>, "ReadType copies bytes, so only trivially copyable types can be read");
		TType value{};
		byteStream.ReadBytes(&value, sizeof(TType));
		return value;
	}

	template<typename TDataType>
	std::vector<TDataType> ReadArray(ByteStream& byteStream)
	{
		static_assert(std::is_trivially_copyable_v<TDataType>, "ReadArray copies bytes, so only trivially copyable types can be read");
		std::vector<TDataType> data(ReadArrayCount(byteStream, sizeof(TDataType)));
		byteStream.ReadBytes(data.data(), data.size() * sizeof(TDataType));
		return data;
	}
}
