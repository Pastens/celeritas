//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RelativisticBremInteractor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Collection.hh"
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

#include "RelativisticBremData.hh"
#include "RBEnergySampler.hh"
#include "BremFinalStateHelper.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Perform a high-energy Bremsstrahlung interaction.
 *
 * This is a relativistic Bremsstrahlung model for high-energy (> 1 GeV)
 * electrons and positrons.
 *
 * \note This performs the same sampling routine as in Geant4's
 * G4eBremsstrahlungRelModel class, as documented in section 10.2.2 of the
 * Geant4 Physics Reference (release 10.7).
 */
class RelativisticBremInteractor
{
  public:
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
    RelativisticBremInteractor(const RelativisticBremNativeRef& shared,
                               const ParticleTrackView&         particle,
                               const Real3&                     direction,
                               const CutoffView&                cutoffs,
                               StackAllocator<Secondary>&       allocate,
                               const MaterialView&              material,
                               const ElementComponentId&        elcomp_id);

    // Sample an interaction with the given RNG
    template<class Engine>
    inline CELER_FUNCTION Interaction operator()(Engine& rng);

  private:
    //// DATA ////

    // Shared constant physics properties
    const RelativisticBremNativeRef& shared_;
    // Incident particle energy
    const Energy inc_energy_;
    // Incident particle momentum
    const Momentum inc_momentum_;
    // Incident direction
    const Real3& inc_direction_;
    // Production cutoff for gammas
    const Energy gamma_cutoff_;
    // Allocate space for a secondary particle
    StackAllocator<Secondary>& allocate_;

    //// HELPER CLASSES ////

    // A helper to sample the photon energy from the relativistic model
    RBEnergySampler rb_energy_sampler_;
    // A helper to update the final state of the primary and the secondary
    BremFinalStateHelper final_state_interaction_;
};

//---------------------------------------------------------------------------//
} // namespace detail
} // namespace celeritas

#include "RelativisticBremInteractor.i.hh"
