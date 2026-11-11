#include <doctest/doctest.h>

#include <Utils/Memory/ByteStream.h>
#include <Utils/Memory/ByteStreamUtils.h>

#include <TestFolder.h>

#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace
{
	void AppendSize(std::vector<Byte>& bytes, size_t size)
	{
		const Byte* const sizeBytes = reinterpret_cast<const Byte*>(&size);
		bytes.insert(bytes.end(), sizeBytes, sizeBytes + sizeof(size));
	}

	// Reads bytes into stream through a file, the way the importers read
	void Load(ByteStream& stream, const ScopedTestFolder& folder, const std::vector<Byte>& bytes)
	{
		const Filepath file = folder.GetFile("Stream.bin");
		{
			std::ofstream output(file.GetAbsolutePath(), std::ios::binary | std::ios::trunc);
			output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		}
		REQUIRE(stream.ReadFromFile(file));
	}
}

TEST_CASE("Reading past the end of a byte stream fails, zero-fills, and stays failed")
{
	ByteStream stream("Test", 6);

	U32 fits = 0xFFFFFFFF;
	CHECK(stream.ReadBytes(&fits, sizeof(fits)));
	CHECK_FALSE(stream.HasFailed());
	CHECK(stream.GetRemainingLength() == 2);

	// 4 more than the 2 left: nothing is read, and the buffer is zeroed rather than left as it was
	U32 pastEnd = 0x12345678;
	CHECK_FALSE(stream.ReadBytes(&pastEnd, sizeof(pastEnd)));
	CHECK(pastEnd == 0);
	CHECK(stream.HasFailed());
	CHECK(stream.GetRemainingLength() == 2);
	CHECK(stream.PeekBytesAdvance(3) == nullptr);

	// A read that fits doesn't clear it
	U16 lastTwo = 0;
	CHECK(stream.ReadBytes(&lastTwo, sizeof(lastTwo)));
	CHECK(stream.HasFailed());
}

TEST_CASE("Writing past the end of a byte stream writes nothing and fails")
{
	ByteStream stream("Test", 4);
	const U16 first = 0x1111;
	REQUIRE(stream.WriteBytes(&first, sizeof(first)));

	const U32 tooBig = 0x22222222;
	CHECK_FALSE(stream.WriteBytes(&tooBig, sizeof(tooBig)));
	CHECK(stream.HasFailed());
	CHECK(stream.GetNumBytesWritten() == sizeof(first));
	const Byte* const bytes = stream.GetBytes();
	CHECK(bytes[2] == 0);
	CHECK(bytes[3] == 0);
}

TEST_CASE("Nothing read at the end of a byte stream isn't a failure")
{
	ByteStream stream("Test", 2);
	U16 all = 0;
	REQUIRE(stream.ReadBytes(&all, sizeof(all)));

	CHECK(stream.PeekBytes() == nullptr);
	stream.PeekBytesAdvance(0);
	CHECK(stream.ReadBytes(nullptr, 0));
	CHECK_FALSE(stream.HasFailed());

	// An empty stream too
	ByteStream empty("Empty");
	CHECK(empty.ReadBytes(nullptr, 0));
	CHECK_FALSE(empty.HasFailed());
}

TEST_CASE("Strings and arrays read back as written")
{
	const ScopedTestFolder folder("UnitTestsTemp_ByteStream");

	std::vector<Byte> bytes;
	const std::string text = "Sponza";
	AppendSize(bytes, text.size());
	bytes.insert(bytes.end(), text.begin(), text.end());
	const float values[] = { 1.0f, 2.5f, -3.0f };
	AppendSize(bytes, sizeof(values));
	const Byte* const valueBytes = reinterpret_cast<const Byte*>(values);
	bytes.insert(bytes.end(), valueBytes, valueBytes + sizeof(values));
	// An empty one at the very end
	AppendSize(bytes, 0);

	ByteStream stream("Test");
	Load(stream, folder, bytes);
	CHECK(ByteStreamUtils::ReadString(stream) == text);
	CHECK(ByteStreamUtils::ReadArray<float>(stream) == std::vector<float>(std::begin(values), std::end(values)));
	CHECK(ByteStreamUtils::ReadString(stream).empty());
	CHECK_FALSE(stream.HasFailed());
	CHECK(stream.GetRemainingLength() == 0);
}

TEST_CASE("A string or array whose size is more than what's left fails without allocating it")
{
	const ScopedTestFolder folder("UnitTestsTemp_ByteStream");

	// Allocating either would throw (it's more than any allocator gives), so passing shows they weren't
	for (const size_t claimedSize : { size_t(9), std::numeric_limits<size_t>::max() })
	{
		CAPTURE(claimedSize);
		std::vector<Byte> bytes;
		AppendSize(bytes, claimedSize);
		bytes.insert(bytes.end(), 8, Byte(0x41));

		ByteStream stringStream("String");
		Load(stringStream, folder, bytes);
		CHECK(ByteStreamUtils::ReadString(stringStream).empty());
		CHECK(stringStream.HasFailed());

		ByteStream arrayStream("Array");
		Load(arrayStream, folder, bytes);
		CHECK(ByteStreamUtils::ReadArray<U32>(arrayStream).empty());
		CHECK(arrayStream.HasFailed());
	}
}

TEST_CASE("An array whose size isn't a whole number of elements fails")
{
	const ScopedTestFolder folder("UnitTestsTemp_ByteStream");

	// 7 bytes of floats: reading them into the 1 float that fits used to overrun it by 3
	std::vector<Byte> bytes;
	AppendSize(bytes, 7);
	bytes.insert(bytes.end(), 7, Byte(0x41));

	ByteStream stream("Test");
	Load(stream, folder, bytes);
	CHECK(ByteStreamUtils::ReadArray<float>(stream).empty());
	CHECK(stream.HasFailed());
}

TEST_CASE("ReadType copies values from bytes that aren't aligned for them")
{
	const ScopedTestFolder folder("UnitTestsTemp_ByteStream");

	std::vector<Byte> bytes = { Byte(0xAB) };
	AppendSize(bytes, 0x0102030405060708);

	ByteStream stream("Test");
	Load(stream, folder, bytes);
	CHECK(ByteStreamUtils::ReadType<U8>(stream) == 0xAB);
	CHECK(ByteStreamUtils::ReadType<size_t>(stream) == 0x0102030405060708);
	CHECK_FALSE(stream.HasFailed());
}
