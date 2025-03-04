//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file PlaneAligned.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Array.hh"
#include "base/Macros.hh"
#include "base/Span.hh"
#include "../Types.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Axis-aligned plane with positive-facing normal.
 */
template<Axis T>
class PlaneAligned
{
  public:
    //@{
    //! Type aliases
    using Intersections  = Array<real_type, 1>;
    using Storage        = Span<const real_type, 1>;
    //@}

    //// CLASS ATTRIBUTES ////

    // Surface type identifier
    static CELER_CONSTEXPR_FUNCTION SurfaceType surface_type();

  public:
    //// CONSTRUCTORS ////

    // Construct with radius
    explicit inline CELER_FUNCTION PlaneAligned(real_type position);

    // Construct from raw data
    explicit inline CELER_FUNCTION PlaneAligned(Storage);

    //// ACCESSORS ////

    //! Get the square of the radius
    CELER_FUNCTION real_type position() const { return position_; }

    //! Get a view to the data for type-deleted storage
    CELER_FUNCTION Storage data() const { return {&position_, 1}; }

    //// CALCULATION ////

    // Determine the sense of the position relative to this surface
    inline CELER_FUNCTION SignedSense calc_sense(const Real3& pos) const;

    // Calculate all possible straight-line intersections with this surface
    inline CELER_FUNCTION Intersections calc_intersections(
        const Real3& pos, const Real3& dir, SurfaceState on_surface) const;

    // Calculate outward normal at a position
    inline CELER_FUNCTION Real3 calc_normal(const Real3& pos) const;

  private:
    //! Intersection with the axis
    real_type position_;

    static CELER_CONSTEXPR_FUNCTION int t_index()
    {
        return static_cast<int>(T);
    }
};

//---------------------------------------------------------------------------//
// TYPE ALIASES
//---------------------------------------------------------------------------//

using PlaneX = PlaneAligned<Axis::x>;
using PlaneY = PlaneAligned<Axis::y>;
using PlaneZ = PlaneAligned<Axis::z>;

//---------------------------------------------------------------------------//
// INLINE FUNCTION DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Surface type identifier.
 */
template<Axis T>
CELER_CONSTEXPR_FUNCTION SurfaceType PlaneAligned<T>::surface_type()
{
    return (T == Axis::x ? SurfaceType::px
                         : (T == Axis::y ? SurfaceType::py : SurfaceType::pz));
}

//---------------------------------------------------------------------------//
/*!
 * Construct from axis intercept.
 */
template<Axis T>
CELER_FUNCTION PlaneAligned<T>::PlaneAligned(real_type position)
    : position_(position)
{
}

//---------------------------------------------------------------------------//
/*!
 * Construct from raw data.
 */
template<Axis T>
CELER_FUNCTION PlaneAligned<T>::PlaneAligned(Storage data) : position_(data[0])
{
}

//---------------------------------------------------------------------------//
/*!
 * Determine the sense of the position relative to this surface.
 */
template<Axis T>
CELER_FUNCTION SignedSense PlaneAligned<T>::calc_sense(const Real3& pos) const
{
    return real_to_sense(pos[t_index()] - position_);
}

//---------------------------------------------------------------------------//
/*!
 * Calculate all possible straight-line intersections with this surface.
 */
template<Axis T>
CELER_FUNCTION auto
PlaneAligned<T>::calc_intersections(const Real3& pos,
                                    const Real3& dir,
                                    SurfaceState on_surface) const
    -> Intersections
{
    if (on_surface == SurfaceState::off && dir[t_index()] != 0)
    {
        real_type dist = (position_ - pos[t_index()]) / dir[t_index()];
        if (dist > 0)
        {
            return {dist};
        }
    }
    return {no_intersection()};
}

//---------------------------------------------------------------------------//
/*!
 * Calculate outward normal at a position.
 */
template<Axis T>
CELER_FUNCTION Real3 PlaneAligned<T>::calc_normal(const Real3&) const
{
    Real3 norm{0, 0, 0};

    norm[t_index()] = 1.;
    return norm;
}

//---------------------------------------------------------------------------//
} // namespace celeritas
