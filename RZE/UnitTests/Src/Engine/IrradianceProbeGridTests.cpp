#include <doctest/doctest.h>

#include <Graphics/IrradianceProbeGrid.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Vector3D.h>

namespace
{
	// Every corner of bounds is at least one spacing inside the grid
	void CheckCovers(const IrradianceProbeGrid& grid, const BoundingBox& bounds)
	{
		const Vector3D farthest = grid.GetProbePosition(grid.CountX - 1, grid.CountY - 1, grid.CountZ - 1);
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(grid.Origin[axis] <= bounds.GetMin()[axis] - grid.Spacing + 0.0001f);
			CHECK(farthest[axis] >= bounds.GetMax()[axis] + grid.Spacing - 0.0001f);
		}
	}
}

TEST_CASE("An empty scene has no probe grid")
{
	CHECK(IrradianceProbeGrid::Fit(BoundingBox(), 4096).IsEmpty());
}

TEST_CASE("The probe spacing follows the scene's size, so big and small scenes get about as many probes")
{
	// Sponza-sized: 30 x 13 x 18 m. The padding included, about the probes asked for, about 1.5 m apart.
	const BoundingBox atrium(Vector3D(-15.0f, 0.0f, -9.0f), Vector3D(15.0f, 13.0f, 9.0f));
	const IrradianceProbeGrid atriumGrid = IrradianceProbeGrid::Fit(atrium, 4096);
	CHECK(atriumGrid.GetProbeCount() == doctest::Approx(4096).epsilon(0.1));
	CHECK(atriumGrid.Spacing > 1.3f);
	CHECK(atriumGrid.Spacing < 1.8f);
	CheckCovers(atriumGrid, atrium);

	// The same scene ten times smaller: ten times closer together, about the same number of probes
	const BoundingBox model(Vector3D(-1.5f, 0.0f, -0.9f), Vector3D(1.5f, 1.3f, 0.9f));
	const IrradianceProbeGrid modelGrid = IrradianceProbeGrid::Fit(model, 4096);
	CHECK(modelGrid.Spacing == doctest::Approx(atriumGrid.Spacing / 10.0f).epsilon(0.05));
	CHECK(modelGrid.GetProbeCount() == doctest::Approx(atriumGrid.GetProbeCount()).epsilon(0.1));
	CheckCovers(modelGrid, model);
}

TEST_CASE("The probe spacing stays within its limits, except to keep under the most probes")
{
	// A tabletop is held at the smallest spacing, so it gets fewer probes than asked for
	const BoundingBox tabletop(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 0.5f, 1.0f));
	const IrradianceProbeGrid tabletopGrid = IrradianceProbeGrid::Fit(tabletop, 4096);
	CHECK(tabletopGrid.Spacing == doctest::Approx(IrradianceProbeGrid::k_minSpacing));
	CHECK(tabletopGrid.GetProbeCount() < 4096);
	CheckCovers(tabletopGrid, tabletop);

	// A flat scene has no volume but still gets probes above and below it
	const BoundingBox floor(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(20.0f, 0.0f, 20.0f));
	const IrradianceProbeGrid floorGrid = IrradianceProbeGrid::Fit(floor, 4096);
	CHECK(floorGrid.CountY >= IrradianceProbeGrid::k_minProbesPerAxis);
	CHECK(floorGrid.GetProbeCount() <= IrradianceProbeGrid::k_maxProbes);
	CheckCovers(floorGrid, floor);

	// A huge scene spreads its probes past the largest spacing rather than over the most probes
	const BoundingBox city(Vector3D(-1000.0f, 0.0f, -1000.0f), Vector3D(1000.0f, 200.0f, 1000.0f));
	const IrradianceProbeGrid cityGrid = IrradianceProbeGrid::Fit(city, 4096);
	CHECK(cityGrid.Spacing > IrradianceProbeGrid::k_maxSpacing);
	CHECK(cityGrid.GetProbeCount() <= IrradianceProbeGrid::k_maxProbes);
	CheckCovers(cityGrid, city);

	// Asking for more than the most probes gets the most at most
	const IrradianceProbeGrid greedyGrid = IrradianceProbeGrid::Fit(tabletop, 1000000);
	CHECK(greedyGrid.GetProbeCount() <= IrradianceProbeGrid::k_maxProbes);
}

TEST_CASE("Probe positions step by the spacing from the origin")
{
	const IrradianceProbeGrid grid = IrradianceProbeGrid::Fit(BoundingBox(Vector3D(0.0f), Vector3D(10.0f)), 1000);
	const Vector3D position = grid.GetProbePosition(2, 3, 4);
	CHECK(position.X() == doctest::Approx(grid.Origin.X() + 2.0f * grid.Spacing));
	CHECK(position.Y() == doctest::Approx(grid.Origin.Y() + 3.0f * grid.Spacing));
	CHECK(position.Z() == doctest::Approx(grid.Origin.Z() + 4.0f * grid.Spacing));
}
