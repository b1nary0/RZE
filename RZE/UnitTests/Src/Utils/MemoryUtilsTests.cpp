#include <doctest/doctest.h>

#include <Utils/Memory/MemoryUtils.h>

TEST_CASE("AlignSize rounds up to the boundary, and leaves a multiple of it alone")
{
	CHECK(MemoryUtils::AlignSize(0, 16) == 0);
	CHECK(MemoryUtils::AlignSize(1, 16) == 16);
	CHECK(MemoryUtils::AlignSize(16, 16) == 16);
	CHECK(MemoryUtils::AlignSize(17, 16) == 32);
	CHECK(MemoryUtils::AlignSize(5, 1) == 5);
}

TEST_CASE("AlignSize keeps sizes past 4 GB")
{
	const size_t fiveGB = MemoryUtils::Gigabytes(5);
	CHECK(MemoryUtils::AlignSize(fiveGB + 1, 256) == fiveGB + 256);
}
