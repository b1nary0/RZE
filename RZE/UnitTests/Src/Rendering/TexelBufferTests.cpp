#include <doctest/doctest.h>

#include <Rendering/RenderCommand.h>
#include <Rendering/TexelBuffer.h>

#include <vector>

namespace
{
	// What CountingFree was given, oldest first. It's a plain function, like stbi_image_free, so the count is shared.
	std::vector<void*> s_freed;

	void CountingFree(void* texels)
	{
		s_freed.push_back(texels);
	}

	// Distinct addresses to hand out as texels; nothing reads them
	U8 s_texelsA[4];
	U8 s_texelsB[4];
}

TEST_CASE("Texels are freed once, when the buffer that holds them goes")
{
	s_freed.clear();
	{
		Rendering::TexelBuffer texels(s_texelsA, &CountingFree);
		CHECK(texels.GetTexels() == s_texelsA);
		CHECK(s_freed.empty());
	}
	REQUIRE(s_freed.size() == 1);
	CHECK(s_freed[0] == s_texelsA);
}

TEST_CASE("Moving texels hands them over without freeing them")
{
	s_freed.clear();
	{
		Rendering::TexelBuffer source(s_texelsA, &CountingFree);
		Rendering::TexelBuffer moved(std::move(source));
		CHECK(source.IsEmpty());
		CHECK(moved.GetTexels() == s_texelsA);
		CHECK(s_freed.empty());

		// Assigning frees what was there before
		Rendering::TexelBuffer other(s_texelsB, &CountingFree);
		other = std::move(moved);
		REQUIRE(s_freed.size() == 1);
		CHECK(s_freed[0] == s_texelsB);
		CHECK(other.GetTexels() == s_texelsA);
	}
	REQUIRE(s_freed.size() == 2);
	CHECK(s_freed[1] == s_texelsA);
}

TEST_CASE("Resetting texels frees them straight away, and an empty buffer frees nothing")
{
	s_freed.clear();
	Rendering::TexelBuffer texels(s_texelsA, &CountingFree);
	texels.Reset();
	CHECK(texels.IsEmpty());
	CHECK(s_freed.size() == 1);

	texels.Reset();
	{
		Rendering::TexelBuffer empty;
	}
	CHECK(s_freed.size() == 1);
}

TEST_CASE("A texture's upload command frees the texels it was given when it's destroyed")
{
	s_freed.clear();
	{
		Rendering::RenderCommand_CreateTextureBuffer2D command;
		command.texels = Rendering::TexelBuffer(s_texelsA, &CountingFree);
		CHECK(s_freed.empty());
	}
	REQUIRE(s_freed.size() == 1);
	CHECK(s_freed[0] == s_texelsA);
}
