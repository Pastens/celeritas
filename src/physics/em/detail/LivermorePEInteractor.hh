//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file LivermorePEInteractor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Algorithms.hh"
#include "base/Macros.hh"
#include "base/StackAllocator.hh"
#include "base/Types.hh"
#include "physics/base/CutoffView.hh"
#include "physics/base/Interaction.hh"
#include "physics/base/ParticleTrackView.hh"
#include "physics/base/Secondary.hh"
#include "physics/base/Units.hh"
#include "physics/em/AtomicRelaxationHelper.hh"
#include "LivermorePEData.hh"
#include "LivermorePEMicroXsCalculator.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Livermore model for the photoelectric effect.
 *
 * A parameterization of the photoelectric cross sections in two different
 * energy intervals, formulated as
 * \f[
 * \sigma(E)
     = a_1 / E + a_2 / E^2 + a_3 / E^3 + a_4 / E^4 + a_5 / E^5 + a_6 / E^6
     \: ,
 * \f]
 * is used. The coefficients for this model are calculated by fitting the
 * tabulated EPICS2014 subshell cross sections. The parameterized model applies
 * above approximately 5 keV; below  this limit (which depends on the atomic
 * number) the tabulated cross sections are used. The angle of the emitted
 * photoelectron is sampled from the Sauter-Gavrila distribution.
 *
 * \note This performs the same sampling routine as in Geant4's
 * G4LivermorePhotoElectricModel class, as documented in section 6.3.5 of the
 * Geant4 Physics Reference (release 10.6).
 */
class LivermorePEInteractor
{
  public:
    //!@{
    //! Type aliases
    using MevEnergy = units::MevEnergy;
    //!@}

  public:
    // Construct with shared and state data
    inline CELER_FUNCTION
    LivermorePEInteractor(const LivermorePERef&         shared,
                          const AtomicRelaxationHelper& relaxation,
                          ElementId                     el_id,
                          const ParticleTrackView&      particle,
                          const CutoffView&             cutoffs,
                          const Real3&                  inc_direction,
                          StackAllocator<Secondary>&    allocate);

    // Sample an interaction with the given RNG
    template<class Engine>
    inline CELER_FUNCTION Interaction operator()(Engine& rng);

  private:
    //// DATA ////

    // Shared constant physics properties
    const LivermorePERef& shared_;
    // Shared scratch space
    const AtomicRelaxationHelper& relaxation_;
    // Index in MaterialParams elements
    ElementId el_id_;
    // Production thresholds
    const CutoffView& cutoffs_;
    // Incident direction
    const Real3& inc_direction_;
    // Incident gamma energy
    const MevEnergy inc_energy_;
    // Allocate space for one or more secondary particles
    StackAllocator<Secondary>& allocate_;
    // Microscopic cross section calculator
    LivermorePEMicroXsCalculator calc_micro_xs_;
    // Reciprocal of the energy
    real_type inv_energy_;

    //// HELPER FUNCTIONS ////

    // Sample the atomic subshel
    template<class Engine>
    inline CELER_FUNCTION SubshellId sample_subshell(Engine& rng) const;

    // Sample the direction of the emitted photoelectron
    template<class Engine>
    inline CELER_FUNCTION Real3 sample_direction(Engine& rng) const;
};

//---------------------------------------------------------------------------//
} // namespace detail
} // namespace celeritas

#include "LivermorePEInteractor.i.hh"
