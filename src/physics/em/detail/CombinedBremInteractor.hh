//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file CombinedBremInteractor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Macros.hh"
#include "base/Types.hh"
#include "base/StackAllocator.hh"

#include "physics/base/CutoffView.hh"
#include "physics/base/Interaction.hh"
#include "physics/base/ParticleTrackView.hh"
#include "physics/base/Secondary.hh"
#include "physics/base/Types.hh"
#include "physics/base/Units.hh"

#include "physics/material/ElementView.hh"
#include "physics/material/MaterialView.hh"
#include "physics/material/Types.hh"

#include "CombinedBremData.hh"
#include "RBEnergySampler.hh"
#include "SBEnergySampler.hh"
#include "BremFinalStateHelper.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Apply either Seltzer-Berger or Relativistic depending on energy.
 *
 * This is a combined bremsstrahlung interactor consisted of the Seltzer-Berger
 * interactor at the low energy (< 1 GeV) and the relativistic bremsstrahlung
 * interactor at the high energy for the e-/e+ bremsstrahlung process.
 */
class CombinedBremInteractor
{
    //!@{
    //! Type aliases
    using Energy      = units::MevEnergy;
    using Momentum    = units::MevMomentum;
    using ElementData = detail::RelBremElementData;
    using ItemIdT     = celeritas::ItemId<unsigned int>;
    //!@}

  public:
    // Construct with shared and state data
    inline CELER_FUNCTION
    CombinedBremInteractor(const CombinedBremNativeRef& shared,
                           const ParticleTrackView&     particle,
                           const Real3&                 direction,
                           const CutoffView&            cutoffs,
                           StackAllocator<Secondary>&   allocate,
                           const MaterialView&          material,
                           const ElementComponentId&    elcomp_id);

    // Sample an interaction with the given RNG
    template<class Engine>
    inline CELER_FUNCTION Interaction operator()(Engine& rng);

  private:
    //// DATA ////

    // Incident particle energy
    const Energy inc_energy_;
    // Incident particle direction
    const Momentum inc_momentum_;
    // Incident particle direction
    const Real3& inc_direction_;
    // Production cutoff for gammas
    const Energy gamma_cutoff_;
    // Allocate space for a secondary particle
    StackAllocator<Secondary>& allocate_;
    // Element in which interaction occurs
    const ElementComponentId elcomp_id_;
    // Incident particle flag for selecting XS correction factor
    const bool is_electron_;
    // Flag for selecting the relativistic bremsstrahlung model
    const bool is_relativistic_;

    //// HELPER CLASSES ////

    // A helper to Sample the photon energy from the relativistic model
    RBEnergySampler rb_energy_sampler_;
    // A helper to sample the photon energy from the SeltzerBerger model
    SBEnergySampler sb_energy_sampler_;
    // A helper to update the final state of the primary and the secondary
    BremFinalStateHelper final_state_interaction_;
};

//---------------------------------------------------------------------------//
} // namespace detail
} // namespace celeritas

#include "CombinedBremInteractor.i.hh"
