#include <doctest/doctest.h>

#include <Graphics/CubeMap.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <cmath>

namespace
{
	struct FaceTexel
	{
		U32 Face = 0;
		// From the face's top-left, 0 to 1
		float U = 0.0f;
		float V = 0.0f;
	};

	// Where the GPU reads a cube texture for a sample direction: D3D's face selection and (s, t) table
	FaceTexel SampleCube(const Vector3D& direction)
	{
		const float x = direction.X();
		const float y = direction.Y();
		const float z = direction.Z();

		FaceTexel texel;
		float sc = 0.0f;
		float tc = 0.0f;
		float ma = 0.0f;
		if (std::abs(x) >= std::abs(y) && std::abs(x) >= std::abs(z))
		{
			texel.Face = x > 0.0f ? 0 : 1;
			sc = x > 0.0f ? -z : z;
			tc = -y;
			ma = std::abs(x);
		}
		else if (std::abs(y) >= std::abs(z))
		{
			texel.Face = y > 0.0f ? 2 : 3;
			sc = x;
			tc = y > 0.0f ? z : -z;
			ma = std::abs(y);
		}
		else
		{
			texel.Face = z > 0.0f ? 4 : 5;
			sc = z > 0.0f ? x : -x;
			tc = -y;
			ma = std::abs(z);
		}

		texel.U = (sc / ma + 1.0f) * 0.5f;
		texel.V = (tc / ma + 1.0f) * 0.5f;
		return texel;
	}

	// Where rendering towards worldDirection through the face lands in its image, with D3D's viewport (v runs down)
	FaceTexel RenderThroughFace(const Vector3D& worldDirection, ECubeFace face)
	{
		const Vector3D origin(2.0f, -1.0f, 5.0f);
		const Matrix4x4 viewProjection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(90.0f, 1.0f, 0.1f, 100.0f)
			* CubeMap::CreateFaceView(origin, face);
		const Vector4D clip = viewProjection * Vector4D(origin + worldDirection, 1.0f);

		FaceTexel texel;
		texel.Face = static_cast<U32>(face);
		texel.U = (clip.X() / clip.W() + 1.0f) * 0.5f;
		texel.V = (1.0f - clip.Y() / clip.W()) * 0.5f;
		return texel;
	}
}

TEST_CASE("Each cube face's view looks straight down the face's direction")
{
	for (U32 faceIndex = 0; faceIndex < static_cast<U32>(ECubeFace::Count); ++faceIndex)
	{
		const ECubeFace face = static_cast<ECubeFace>(faceIndex);
		const Vector4D ahead = CubeMap::CreateFaceView(Vector3D(), face) * Vector4D(CubeMap::GetFaceForward(face), 0.0f);

		CHECK(ahead.X() == doctest::Approx(0.0f));
		CHECK(ahead.Y() == doctest::Approx(0.0f));
		// View space looks down -Z
		CHECK(ahead.Z() == doctest::Approx(-1.0f));
	}
}

TEST_CASE("What a cube face renders is what sampling the cube reads, for every direction")
{
	// A spread of directions over the whole sphere, avoiding the edges between faces where either face would do
	int checkedCount = 0;
	for (int i = -4; i <= 4; ++i)
	{
		for (int j = -4; j <= 4; ++j)
		{
			for (int k = -4; k <= 4; ++k)
			{
				const Vector3D direction(i + 0.13f, j - 0.29f, k + 0.41f);

				const FaceTexel sampled = SampleCube(CubeMap::ToSampleDirection(direction));
				const FaceTexel rendered = RenderThroughFace(direction, static_cast<ECubeFace>(sampled.Face));

				// The face sampling picks is the one looking most nearly along the direction
				const Vector3D sampledForward = CubeMap::GetFaceForward(static_cast<ECubeFace>(sampled.Face));
				for (U32 face = 0; face < static_cast<U32>(ECubeFace::Count); ++face)
				{
					CHECK(VectorUtils::Dot(direction, sampledForward) >= VectorUtils::Dot(direction, CubeMap::GetFaceForward(static_cast<ECubeFace>(face))) - 0.0001f);
				}

				CHECK(rendered.U == doctest::Approx(sampled.U).epsilon(0.0001));
				CHECK(rendered.V == doctest::Approx(sampled.V).epsilon(0.0001));
				++checkedCount;
			}
		}
	}
	CHECK(checkedCount == 729);
}

TEST_CASE("A cube face sees a box only if the box reaches into its 90-degree view, in the cube's face order")
{
	const Vector3D centre(10.0f, 2.0f, -3.0f);
	const auto facesSeeing = [&centre](const Vector3D& min, const Vector3D& max)
	{
		return CubeMap::GetFacesSeeing(centre, BoundingBox(centre + min, centre + max));
	};
	const auto bit = [](ECubeFace face) { return 1u << static_cast<U32>(face); };

	// Small and straight ahead of each face: that face only
	for (U32 face = 0; face < static_cast<U32>(ECubeFace::Count); ++face)
	{
		const Vector3D ahead = CubeMap::GetFaceForward(static_cast<ECubeFace>(face)) * 5.0f;
		CHECK(facesSeeing(ahead - Vector3D(0.5f), ahead + Vector3D(0.5f)) == 1u << face);
	}

	// Around the centre: every face
	CHECK(facesSeeing(Vector3D(-1.0f), Vector3D(1.0f)) == (1u << static_cast<U32>(ECubeFace::Count)) - 1);
	// Ahead and up at 45 degrees: on the edge between Right and Up, so both
	CHECK(facesSeeing(Vector3D(5.0f, 4.5f, -0.5f), Vector3D(6.0f, 5.5f, 0.5f)) == (bit(ECubeFace::Right) | bit(ECubeFace::Up)));
	// Far off to the side of Right's view, though ahead of it: only Up sees it
	CHECK(facesSeeing(Vector3D(1.0f, 5.0f, -0.5f), Vector3D(2.0f, 6.0f, 0.5f)) == bit(ECubeFace::Up));
	// Nothing to see
	CHECK(CubeMap::GetFacesSeeing(centre, BoundingBox()) == 0);
}

TEST_CASE("A face's view sees nothing behind it")
{
	const Vector3D forward = CubeMap::GetFaceForward(ECubeFace::Forward);
	const Vector3D behind = forward * -5.0f;
	CHECK_FALSE(CubeMap::IsInFaceView(forward, BoundingBox(behind - Vector3D(0.5f), behind + Vector3D(0.5f))));
	// Touching the view's plane from behind isn't in front of it
	CHECK_FALSE(CubeMap::IsInFaceView(forward, BoundingBox(Vector3D(-1.0f, -1.0f, 0.0f), Vector3D(1.0f, 1.0f, 2.0f))));
	CHECK(CubeMap::IsInFaceView(forward, BoundingBox(Vector3D(-1.0f, -1.0f, -2.0f), Vector3D(1.0f, 1.0f, 2.0f))));
}
