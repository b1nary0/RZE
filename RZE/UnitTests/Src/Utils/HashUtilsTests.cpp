#include <doctest/doctest.h>

#include <Utils/HashUtils.h>

TEST_CASE("HashFloats: the same values hash the same")
{
	CHECK(HashUtils::HashFloats({ 1.0f, 2.5f, 3.0f }) == HashUtils::HashFloats({ 1.0f, 2.5f, 3.0f }));
}

TEST_CASE("HashFloats: a different value hashes differently")
{
	CHECK(HashUtils::HashFloats({ 1.0f, 2.5f, 3.0f }) != HashUtils::HashFloats({ 1.0f, 2.5f, 3.5f }));
	CHECK(HashUtils::HashFloats({ 1.0f, 2.5f, 3.0f }) != HashUtils::HashFloats({ 0.0f, 2.5f, 3.0f }));
}

TEST_CASE("HashFloats: the order counts")
{
	CHECK(HashUtils::HashFloats({ 1.0f, 2.0f }) != HashUtils::HashFloats({ 2.0f, 1.0f }));
}

TEST_CASE("HashFloats: the count counts")
{
	CHECK(HashUtils::HashFloats({ 1.0f }) != HashUtils::HashFloats({ 1.0f, 0.0f }));
	CHECK(HashUtils::HashFloats({ 0.0f }) != HashUtils::HashFloats({}));
}

TEST_CASE("HashFloats: negative zero isn't zero, as the bits differ")
{
	CHECK(HashUtils::HashFloats({ -0.0f }) != HashUtils::HashFloats({ 0.0f }));
}

TEST_CASE("HashFloats: no values hash to FNV-1a's offset basis")
{
	CHECK(HashUtils::HashFloats({}) == 14695981039346656037ull);
}
