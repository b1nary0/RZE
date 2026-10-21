#include <doctest/doctest.h>

#include <Windowing/WindowIcon.h>

#include <initializer_list>

namespace
{
	IconImage MakeImage(int width, int height, std::initializer_list<U8> texels)
	{
		IconImage image;
		image.Width = width;
		image.Height = height;
		image.Texels.assign(texels);
		return image;
	}

	// The texel at x, y as R, G, B, A
	const U8* TexelAt(const IconImage& image, int x, int y)
	{
		return &image.Texels[(static_cast<size_t>(y) * image.Width + x) * 4];
	}
}

TEST_CASE("WindowIcon::MakeSquare averages the texels each output texel covers")
{
	const IconImage image = MakeImage(2, 2, {
		255, 0, 0, 255,		0, 255, 0, 255,
		0, 0, 255, 255,		255, 255, 255, 255 });

	const IconImage square = WindowIcon::MakeSquare(image, 1);
	REQUIRE(square.Width == 1);
	REQUIRE(square.Height == 1);
	const U8* const texel = TexelAt(square, 0, 0);
	CHECK(texel[0] == 128);
	CHECK(texel[1] == 128);
	CHECK(texel[2] == 128);
	CHECK(texel[3] == 255);
}

TEST_CASE("WindowIcon::MakeSquare centres an image that isn't square on clear space")
{
	IconImage image;
	image.Width = 4;
	image.Height = 2;
	image.Texels.assign(4 * 2 * 4, 255);

	const IconImage square = WindowIcon::MakeSquare(image, 4);
	for (int x = 0; x < 4; ++x)
	{
		CHECK(TexelAt(square, x, 0)[3] == 0);
		CHECK(TexelAt(square, x, 1)[3] == 255);
		CHECK(TexelAt(square, x, 2)[3] == 255);
		CHECK(TexelAt(square, x, 3)[3] == 0);
	}
	CHECK(TexelAt(square, 1, 1)[0] == 255);
}

TEST_CASE("WindowIcon::MakeSquare doesn't darken colours next to clear texels")
{
	const IconImage image = MakeImage(2, 2, {
		255, 0, 0, 255,		0, 0, 0, 0,
		0, 0, 0, 0,			255, 0, 0, 255 });

	const IconImage square = WindowIcon::MakeSquare(image, 1);
	const U8* const texel = TexelAt(square, 0, 0);
	CHECK(texel[0] == 255);
	CHECK(texel[1] == 0);
	CHECK(texel[3] == 128);
}

TEST_CASE("WindowIcon::MakeSquare scales a small image up")
{
	const IconImage image = MakeImage(1, 1, { 10, 20, 30, 255 });

	const IconImage square = WindowIcon::MakeSquare(image, 2);
	for (int y = 0; y < 2; ++y)
	{
		for (int x = 0; x < 2; ++x)
		{
			const U8* const texel = TexelAt(square, x, y);
			CHECK(texel[0] == 10);
			CHECK(texel[1] == 20);
			CHECK(texel[2] == 30);
			CHECK(texel[3] == 255);
		}
	}
}
