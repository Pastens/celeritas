//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file InteractorHostTestBase.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <random>
#include <vector>
#include "base/Array.hh"
#include "base/ArrayIO.hh"
#include "base/CollectionStateStore.hh"
#include "base/Span.hh"
#include "base/StackAllocator.hh"
#include "base/Types.hh"
#include "physics/base/ModelIdGenerator.hh"
#include "physics/base/ParticleParams.hh"
#include "physics/base/ParticleData.hh"
#include "physics/base/CutoffParams.hh"
#include "physics/base/CutoffData.hh"
#include "physics/base/Secondary.hh"
#include "physics/base/Units.hh"
#include "physics/material/MaterialParams.hh"
#include "physics/material/MaterialData.hh"

// Test helpers
#include "gtest/Test.hh"
#include "random/DiagnosticRngEngine.hh"

namespace celeritas
{
class ParticleTrackView;
class MaterialTrackView;
struct Interaction;
} // namespace celeritas

namespace celeritas_test
{
//---------------------------------------------------------------------------//
/*!
 * Test harness base class for EM physics models.
 *
 * \todo Since this now uses Collection objects it's generally safe to use this
 * to test Models as well as device code -- think about renaming it.
 */
class InteractorHostTestBase : public celeritas::Test
{
  public:
    static constexpr celeritas::MemSpace host = celeritas::MemSpace::host;
    //!@{
    //! Type aliases
    using RandomEngine = DiagnosticRngEngine<std::mt19937>;

    using real_type = celeritas::real_type;
    using PDGNumber = celeritas::PDGNumber;
    using MevEnergy = celeritas::units::MevEnergy;

    using MaterialId        = celeritas::MaterialId;
    using MaterialParams    = celeritas::MaterialParams;
    using MaterialTrackView = celeritas::MaterialTrackView;

    using CutoffParams = celeritas::CutoffParams;

    using Interaction          = celeritas::Interaction;
    using ModelIdGenerator     = celeritas::ModelIdGenerator;
    using ModelId              = celeritas::ModelId;
    using ParticleId           = celeritas::ParticleId;
    using ParticleParams       = celeritas::ParticleParams;
    using ParticleTrackView    = celeritas::ParticleTrackView;
    using Real3                = celeritas::Real3;
    using Secondary            = celeritas::Secondary;
    using SecondaryAllocator   = celeritas::StackAllocator<Secondary>;
    using constSpanSecondaries = celeritas::Span<const Secondary>;
    //!@}

  public:
    //!@{
    //! Initialize and destroy
    InteractorHostTestBase();
    ~InteractorHostTestBase();
    //!@}

    //!@{
    //! Set and get material properties
    void set_material_params(MaterialParams::Input inp);
    const std::shared_ptr<const MaterialParams>& material_params() const
    {
        CELER_EXPECT(material_params_);
        return material_params_;
    }
    //!@}

    //!@{
    //! Set and get particle params
    void set_particle_params(ParticleParams::Input inp);
    const std::shared_ptr<const ParticleParams>& particle_params() const
    {
        CELER_EXPECT(particle_params_);
        return particle_params_;
    }
    //!@}

    //!@{
    //! Set and get cutoff params
    void set_cutoff_params(CutoffParams::Input inp);
    const std::shared_ptr<const CutoffParams>& cutoff_params() const
    {
        CELER_EXPECT(cutoff_params_);
        return cutoff_params_;
    }
    //!@}

    //!@{
    //! Material properties
    void               set_material(const std::string& name);
    MaterialTrackView& material_track()
    {
        CELER_EXPECT(mt_view_);
        return *mt_view_;
    }
    //!@}

    //!@{
    //! Incident particle properties and access
    void                     set_inc_particle(PDGNumber n, MevEnergy energy);
    void                     set_inc_direction(const Real3& dir);
    const Real3&             direction() const { return inc_direction_; }
    const ParticleTrackView& particle_track() const
    {
        CELER_EXPECT(pt_view_);
        return *pt_view_;
    }
    //!@}

    //!@{
    //! Secondary stack storage and access
    void                resize_secondaries(int count);
    SecondaryAllocator& secondary_allocator()
    {
        CELER_EXPECT(sa_view_);
        return *sa_view_;
    }
    //!@}

    //!@{
    //! Get random number generator with clean counter
    RandomEngine& rng()
    {
        rng_.reset_count();
        return rng_;
    }
    //!@}

    // Check for energy and momentum conservation
    void check_conservation(const Interaction& interaction) const;

    // Check for energy conservation
    void check_energy_conservation(const Interaction& interaction) const;

    // Check for momentum conservation
    void check_momentum_conservation(const Interaction& interaction) const;

  private:
    template<template<celeritas::Ownership, celeritas::MemSpace> class S>
    using StateStore
        = celeritas::CollectionStateStore<S, celeritas::MemSpace::host>;
    template<celeritas::Ownership W, celeritas::MemSpace M>
    using SecondaryStackData
        = celeritas::StackAllocatorData<celeritas::Secondary, W, M>;

    std::shared_ptr<const MaterialParams> material_params_;
    std::shared_ptr<const ParticleParams> particle_params_;
    std::shared_ptr<const CutoffParams>   cutoff_params_;
    RandomEngine                          rng_;

    StateStore<celeritas::MaterialStateData> ms_;
    StateStore<celeritas::ParticleStateData> ps_;

    Real3                          inc_direction_ = {0, 0, 1};
    StateStore<SecondaryStackData> secondaries_;

    // Views
    std::shared_ptr<MaterialTrackView>  mt_view_;
    std::shared_ptr<ParticleTrackView>  pt_view_;
    std::shared_ptr<SecondaryAllocator> sa_view_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas_test
