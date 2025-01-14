//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file Transporter.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/Assert.hh"
#include "base/CollectionStateStore.hh"
#include "base/Types.hh"
#include "sim/TrackData.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
class GeoParams;
class MaterialParams;
class GeoMaterialParams;
class ParticleParams;
class CutoffParams;
class PhysicsParams;
class AtomicRelaxationParams;
class RngParams;
class TrackInitParams;

//---------------------------------------------------------------------------//
//! Input parameters to the transporter.
struct TransporterInput
{
    // Geometry and materials
    std::shared_ptr<const GeoParams>         geometry;
    std::shared_ptr<const MaterialParams>    materials;
    std::shared_ptr<const GeoMaterialParams> geo_mats;

    // Physics
    std::shared_ptr<const ParticleParams>         particles;
    std::shared_ptr<const CutoffParams>           cutoffs;
    std::shared_ptr<const PhysicsParams>          physics;
    std::shared_ptr<const AtomicRelaxationParams> relaxation;

    // Random
    std::shared_ptr<const RngParams> rng;

    // Constants
    size_type max_num_tracks{};
    size_type max_steps{};
    real_type secondary_stack_factor{};

    //! True if all params are assigned
    explicit operator bool() const
    {
        return geometry && materials && geo_mats && particles && cutoffs
               && physics && rng;
    }
};

//---------------------------------------------------------------------------//
//! Tallied result and timing from transporting a set of primaries
struct TransporterResult
{
    //!@{
    //! Type aliases
    using VecCount          = std::vector<size_type>;
    using VecReal           = std::vector<real_type>;
    using MapStringCount    = std::unordered_map<std::string, size_type>;
    using MapStringVecCount = std::unordered_map<std::string, VecCount>;
    //!@}

    //// DATA ////

    VecReal           time;    //!< Real time per step
    VecCount          alive;   //!< Num living tracks per step
    VecReal           edep;    //!< Energy deposition along the grid
    MapStringCount    process; //!< Count of particle/process interactions
    MapStringVecCount steps;   //!< Distribution of steps
    double            total_time = 0; //!< Wall clock
};

//---------------------------------------------------------------------------//
/*!
 * Interface class for transporting a set of primaries to completion.
 *
 * We might want to change this so that the transport result gets accumulated
 * over multiple calls rather than combining for a single operation, so
 * diagnostics would be an acessor and the "call" operator would be renamed
 * "transport". Such a change would imply making the diagnostics part of the
 * input parameters, which (for simplicity) isn't done yet.
 */
class TransporterBase
{
  public:
    virtual ~TransporterBase() = 0;

    // Transport the input primaries and all secondaries produced
    virtual TransporterResult operator()(const TrackInitParams& primaries) = 0;

    //! Access input parameters (TODO hacky)
    virtual const TransporterInput& input() const = 0;
};

//---------------------------------------------------------------------------//
/*!
 * Transport a set of primaries to completion.
 */
template<MemSpace M>
class Transporter : public TransporterBase
{
  public:
    // Construct from parameters
    explicit Transporter(TransporterInput inp);

    // Transport the input primaries and all secondaries produced
    TransporterResult operator()(const TrackInitParams& primaries) final;

    //! Access input parameters (TODO hacky)
    const TransporterInput& input() const final { return input_; }

  private:
    TransporterInput                          input_;
    ParamsData<Ownership::const_reference, M> params_;
    CollectionStateStore<StateData, M>        states_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas
