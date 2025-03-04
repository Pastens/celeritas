//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file SurfaceFunctors.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Algorithms.hh"
#include "base/Array.hh"
#include "base/Assert.hh"
#include "Types.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
//! Calculate the sense of a surface at a given position.
struct CalcSense
{
    const Real3& pos;

    template<class S>
    CELER_FUNCTION SignedSense operator()(S&& surf)
    {
        return surf.calc_sense(this->pos);
    }
};

//---------------------------------------------------------------------------//
//! Get the number of intersections of a surface
template<class S>
struct NumIntersections
{
    constexpr size_type operator()() const noexcept
    {
        return typename S::Intersections{}.size();
    }
};

//---------------------------------------------------------------------------//
//! Calculate the outward normal at a position.
struct CalcNormal
{
    const Real3& pos;

    template<class S>
    CELER_FUNCTION Real3 operator()(S&& surf)
    {
        return surf.calc_normal(this->pos);
    }
};

//---------------------------------------------------------------------------//
/*!
 * Calculate the smallest distance from a point to the surface.
 *
 * I need to check the math on this, as it assumes that the closest point is
 * the closest intersection along the surface normal. There might also be
 * easier ways to check this distance analytically. This isn't yet used
 * anywhere so I haven't made it a priority.
 */
struct CalcSafetyDistance
{
    const Real3& pos;

    //! Operate on a surface
    template<class S>
    real_type operator()(S&& surf)
    {
        // Calculate outward normal
        Real3 dir = surf.calc_normal(this->pos);

        auto sense = surf.calc_sense(this->pos);
        // If sense is "positive" (on or outside), flip direction to inward so
        // that the vector points toward the surface
        if (sense == SignedSense::outside)
        {
            for (int i = 0; i < 3; ++i)
                dir[i] *= -1;
        }
        else if (sense == SignedSense::on)
        {
            return 0;
        }

        // Return the closest intersection
        auto intersect
            = surf.calc_intersections(this->pos, dir, SurfaceState::off);
        return *celeritas::min_element(intersect.begin(), intersect.end());
    }
};

//---------------------------------------------------------------------------//
/*!
 * Fill an array with distances-to-intersection.
 *
 * This assumes that each call is to the next face index, starting with face
 * zero.
 */
class CalcIntersections
{
  public:
    //!@{
    //! Types
    using face_int = FaceId::size_type;
    //!@}

  public:
    //! Construct from the particle point, direction, face ID, and temp storage
    CELER_FUNCTION CalcIntersections(const Real3& pos,
                                     const Real3& dir,
                                     FaceId       on_face,
                                     TempNextFace next_face)
        : pos_(pos)
        , dir_(dir)
        , on_face_idx_(on_face.unchecked_get())
        , face_idx_(0)
        , face_(next_face.face)
        , dist_(next_face.distance)
    {
        CELER_EXPECT(face_ && dist_);
    }

    //! Operate on a surface
    template<class S>
    CELER_FUNCTION void operator()(S&& surf)
    {
        auto on_surface = (on_face_idx_ == face_idx_) ? SurfaceState::on
                                                      : SurfaceState::off;

        // Calculate distance to surface along this direction
        auto all_dist = surf.calc_intersections(pos_, dir_, on_surface);

        // Copy possible intersections and this surface to the output
        for (real_type dist : all_dist)
        {
            CELER_ASSERT(dist >= 0);
            *face_++ = FaceId{face_idx_};
            *dist_++ = dist;
        }
        // Increment to next face
        ++face_idx_;
    }

    CELER_FUNCTION face_int face_idx() const { return face_idx_; }

  private:
    //// DATA ////

    const Real3&   pos_;
    const Real3&   dir_;
    const face_int on_face_idx_;
    face_int       face_idx_;
    FaceId*        face_;
    real_type*     dist_;
};

//---------------------------------------------------------------------------//
} // namespace detail
} // namespace celeritas
