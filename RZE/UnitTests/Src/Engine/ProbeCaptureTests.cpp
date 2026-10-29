#include <doctest/doctest.h>

#include <Graphics/ProbeCapture.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/CubeMap.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <cmath>

namespace
{
	// What ProbeCapture draws a face with (Vertex_ProbeCapture.hlsl, before placing it in the target)
	Matrix4x4 FaceClipSpace(U32 face)
	{
		const ProbeCapture::FaceBasis basis = ProbeCapture::GetFaceBasis(face);
		const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(90.0f, 1.0f, 0.02f, 100.0f);
		return projection * Matrix4x4::CreateViewMatrix(Vector3D(0.0f), basis.Forward, basis.Up);
	}

	// Where direction lands on a face: -1..1 across, y up, and whether it's in front
	bool ProjectOntoFace(U32 face, const Vector3D& direction, float& outNdcX, float& outNdcY)
	{
		const Vector4D clip = FaceClipSpace(face) * Vector4D(direction.X(), direction.Y(), direction.Z(), 1.0f);
		if (clip.W() <= 0.0f)
		{
			return false;
		}
		outNdcX = clip.X() / clip.W();
		outNdcY = clip.Y() / clip.W();
		return std::abs(outNdcX) <= 1.0f && std::abs(outNdcY) <= 1.0f;
	}
}

TEST_CASE("Each probe face's texel directions land on that texel's centre through the face's camera")
{
	constexpr U32 k_faceSize = 8;
	for (U32 face = 0; face < ProbeCapture::k_faceCount; ++face)
	{
		for (U32 y = 0; y < k_faceSize; ++y)
		{
			for (U32 x = 0; x < k_faceSize; ++x)
			{
				const Vector3D direction = ProbeCapture::GetTexelDirection(face, x, y, k_faceSize);
				CHECK(direction.Length() == doctest::Approx(1.0f));

				float ndcX = 0.0f;
				float ndcY = 0.0f;
				REQUIRE(ProjectOntoFace(face, direction, ndcX, ndcY));
				// Texel x from the left, y from the top
				CHECK((ndcX * 0.5f + 0.5f) * k_faceSize == doctest::Approx(x + 0.5f).epsilon(0.0001));
				CHECK((0.5f - ndcY * 0.5f) * k_faceSize == doctest::Approx(y + 0.5f).epsilon(0.0001));
			}
		}

		// A one-texel face looks straight along its forward
		const Vector3D centre = ProbeCapture::GetTexelDirection(face, 0, 0, 1);
		CHECK(VectorUtils::Dot(centre, ProbeCapture::GetFaceBasis(face).Forward) == doctest::Approx(1.0f));
	}
}

TEST_CASE("The six probe faces see every direction between them")
{
	constexpr int k_sampleCount = 500;
	const float goldenAngle = MathUtils::Pi * (3.0f - std::sqrt(5.0f));
	for (int sample = 0; sample < k_sampleCount; ++sample)
	{
		const float y = 1.0f - 2.0f * (static_cast<float>(sample) + 0.5f) / static_cast<float>(k_sampleCount);
		const float radius = std::sqrt(1.0f - y * y);
		const float angle = goldenAngle * static_cast<float>(sample);
		const Vector3D direction(std::cos(angle) * radius, y, std::sin(angle) * radius);

		int facesSeeing = 0;
		for (U32 face = 0; face < ProbeCapture::k_faceCount; ++face)
		{
			float ndcX = 0.0f;
			float ndcY = 0.0f;
			if (ProjectOntoFace(face, direction, ndcX, ndcY))
			{
				++facesSeeing;
			}
		}
		// Only directions exactly on an edge between faces would be seen by two
		CHECK(facesSeeing == 1);
	}
}

TEST_CASE("A probe face sees a box only if the box reaches into its 90-degree view")
{
	const ProbeCapture capture(8, ProbeCapture::EProbeFaces::Probe);
	const Vector3D probe(10.0f, 2.0f, -3.0f);
	const auto facesSeeing = [&capture, &probe](const Vector3D& min, const Vector3D& max)
	{
		return capture.GetFacesSeeing(probe, BoundingBox(probe + min, probe + max));
	};

	// A box around the probe: every face
	CHECK(facesSeeing(Vector3D(-1.0f), Vector3D(1.0f)) == ProbeCapture::k_allFaces);
	// Small and straight ahead of a face (GetFaceBasis's order: +X, -X, +Y, -Y, +Z, -Z): that face only
	CHECK(facesSeeing(Vector3D(5.0f, -0.5f, -0.5f), Vector3D(6.0f, 0.5f, 0.5f)) == 1u << 0);
	CHECK(facesSeeing(Vector3D(-0.5f, 5.0f, -0.5f), Vector3D(0.5f, 6.0f, 0.5f)) == 1u << 2);
	CHECK(facesSeeing(Vector3D(-0.5f, -0.5f, -6.0f), Vector3D(0.5f, 0.5f, -5.0f)) == 1u << 5);
	// Ahead and up at 45 degrees: on the edge between +X and +Y, so both
	CHECK(facesSeeing(Vector3D(5.0f, 4.5f, -0.5f), Vector3D(6.0f, 5.5f, 0.5f)) == ((1u << 0) | (1u << 2)));
	// Far off to the side of +X's view, though ahead of it: only +Y sees it
	CHECK(facesSeeing(Vector3D(1.0f, 5.0f, -0.5f), Vector3D(2.0f, 6.0f, 0.5f)) == 1u << 2);
	CHECK(capture.GetFacesSeeing(probe, BoundingBox()) == 0);
}

TEST_CASE("Reflection probe faces are told apart in CubeMap's order")
{
	const ProbeCapture capture(8, ProbeCapture::EProbeFaces::CubeMap);
	const Vector3D probe(0.0f);
	for (U32 face = 0; face < ProbeCapture::k_faceCount; ++face)
	{
		const Vector3D ahead = CubeMap::GetFaceForward(static_cast<ECubeFace>(face)) * 5.0f;
		CHECK(capture.GetFacesSeeing(probe, BoundingBox(ahead - Vector3D(0.5f), ahead + Vector3D(0.5f))) == 1u << face);
	}
}
