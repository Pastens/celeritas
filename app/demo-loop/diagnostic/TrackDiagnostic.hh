//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file TrackDiagnostic.hh
//---------------------------------------------------------------------------//
#pragma once

#include "Diagnostic.hh"

#include "base/Macros.hh"
#include "physics/base/ModelData.hh"
#include "sim/SimTrackView.hh"
#include "sim/TrackData.hh"

namespace demo_loop
{
//---------------------------------------------------------------------------//
/*!
 * Functor to determine if the given SimTrackState is 'alive'.
 */
struct OneIfAlive
{
    using size_type = celeritas::size_type;

    CELER_FUNCTION size_type operator()(const celeritas::SimTrackState& sim) const
    {
        return sim.alive ? 1 : 0;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Diagnostic class for collecting track data stored in device memory.
 *
 * Collects the number of surviving tracks after a given simulation step
 * through a reduction on a state's tracks.
 */
template<MemSpace M>
class TrackDiagnostic : public Diagnostic<M>
{
  public:
    using size_type    = celeritas::size_type;
    using StateDataRef = celeritas::StateData<Ownership::reference, M>;

    TrackDiagnostic() : Diagnostic<M>() {}

    // Number of alive tracks determined at the end of a step.
    void end_step(const StateDataRef& data) final;

    // Return vector consisting of the number of alive tracks at each step
    // (indexed by step number).
    const std::vector<size_type>& num_alive_per_step() const
    {
        return num_alive_per_step_;
    }

  private:
    std::vector<size_type> num_alive_per_step_;
};

//---------------------------------------------------------------------------//
// KERNEL LAUNCHER(S)
//---------------------------------------------------------------------------//
celeritas::size_type reduce_alive(const celeritas::StateDeviceRef& states);
celeritas::size_type reduce_alive(const celeritas::StateHostRef& states);
} // namespace demo_loop
