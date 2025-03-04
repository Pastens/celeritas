//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RngStateInit.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Collection.hh"

namespace celeritas
{
// Forward declarations to avoid circular header dependency
template<MemSpace M>
struct RngInitializer;
template<Ownership W, MemSpace M>
struct RngStateData;

namespace detail
{
//---------------------------------------------------------------------------//
template<Ownership W, MemSpace M>
struct RngInitData
{
    StateCollection<RngInitializer<M>, W, M> seeds;

    //// METHODS ////

    //! True if assigned
    explicit CELER_FUNCTION operator bool() const { return !seeds.empty(); }

    //! State size
    CELER_FUNCTION size_type size() const { return seeds.size(); }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    RngInitData& operator=(const RngInitData<W2, M2>& other)
    {
        static_assert(M == M2,
                      "seeds state cannot be transferred between host and "
                      "device because they use separate seeds types");
        CELER_EXPECT(other);
        seeds = other.seeds;
        return *this;
    }
};

//---------------------------------------------------------------------------//
// Initialize the RNG state on host/device
void rng_state_init(
    const RngStateData<Ownership::reference, MemSpace::device>&      rng,
    const RngInitData<Ownership::const_reference, MemSpace::device>& seeds);

void rng_state_init(
    const RngStateData<Ownership::reference, MemSpace::host>&      rng,
    const RngInitData<Ownership::const_reference, MemSpace::host>& seeds);

//---------------------------------------------------------------------------//
} // namespace detail
} // namespace celeritas
