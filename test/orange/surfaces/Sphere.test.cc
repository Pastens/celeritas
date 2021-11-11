//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file Sphere.test.cc
//---------------------------------------------------------------------------//
#include "orange/surfaces/Sphere.hh"

#include "base/Constants.hh"
#include "celeritas_test.hh"

using celeritas::no_intersection;
using celeritas::SignedSense;
using celeritas::Sphere;
using celeritas::SurfaceState;

using celeritas::ipow;
using celeritas::Real3;
using celeritas::real_type;

using Intersections = Sphere::Intersections;

constexpr real_type sqrt_third = 1 / celeritas::constants::sqrt_three;

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class SphereTest : public celeritas::Test
{
  protected:
    void SetUp() override {}
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//
/*!
 * This shape is an ellipsoid:
 * - with radii {3, 2, 1},
 * - rotated by 60 degrees about the x axis,
 * - then 30 degrees about z,
 * - and finally translated by {1, 2, 3}.
 */
TEST_F(SphereTest, all)
{
    EXPECT_EQ(celeritas::SurfaceType::s, Sphere::surface_type());
    EXPECT_EQ(4, Sphere::Storage::extent);
    EXPECT_EQ(2, Sphere::Intersections{}.size());

    const Real3 origin{-1.1, 2.2, -3.3};
    real_type   radius = 4.4;

    Sphere s{origin, radius};
    EXPECT_VEC_SOFT_EQ(origin, s.origin());
    EXPECT_SOFT_EQ(radius * radius, s.radius_sq());

    EXPECT_EQ(SignedSense::outside, s.calc_sense({4, 5, 5}));
    EXPECT_EQ(SignedSense::inside, s.calc_sense({1, 2, -3}));

    const Real3 on_surface{origin[0] + radius * sqrt_third,
                           origin[1] + radius * sqrt_third,
                           origin[2] + radius * sqrt_third};
    const Real3 inward{-sqrt_third, -sqrt_third, -sqrt_third};
    const Real3 outward{sqrt_third, sqrt_third, sqrt_third};

    Intersections distances;

    // On surface, inward
    distances = s.calc_intersections(on_surface, inward, SurfaceState::on);
    EXPECT_SOFT_EQ(2 * radius, distances[0]);
    EXPECT_SOFT_EQ(no_intersection(), distances[1]);

    // "Not on surface", inward
    distances = s.calc_intersections(on_surface, inward, SurfaceState::off);
    EXPECT_SOFT_EQ(1e-16, distances[0]);
    EXPECT_SOFT_EQ(2 * radius, distances[1]);

    // On surface, outward
    distances = s.calc_intersections(on_surface, outward, SurfaceState::on);
    EXPECT_SOFT_EQ(no_intersection(), distances[0]);
    EXPECT_SOFT_EQ(no_intersection(), distances[1]);

    // At center
    distances = s.calc_intersections(origin, outward, SurfaceState::off);
    EXPECT_SOFT_EQ(radius, distances[1]);
    EXPECT_SOFT_EQ(no_intersection(), distances[0]);

    // Outside, hitting both
    distances = s.calc_intersections(
        Real3{-6.5, 2.2, -3.3}, Real3{1, 0, 0}, SurfaceState::off);
    EXPECT_SOFT_EQ(1.0, distances[0]);
    EXPECT_SOFT_EQ(1 + 2 * radius, distances[1]);
}