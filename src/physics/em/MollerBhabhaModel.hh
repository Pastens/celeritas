//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file MollerBhabhaModel.hh
//---------------------------------------------------------------------------//
#pragma once

#include "physics/base/Model.hh"
#include "physics/base/ParticleParams.hh"
#include "detail/MollerBhabhaData.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Set up and launch the Moller-Bhabha model interaction.
 */
class MollerBhabhaModel final : public Model
{
  public:
    // Construct from model ID and other necessary data
    MollerBhabhaModel(ModelId id, const ParticleParams& particles);

    // Particle types and energy ranges that this model applies to
    SetApplicability applicability() const final;

    // Apply the interaction kernel on host
    void interact(const HostInteractRef&) const final;

    // Apply the interaction kernel on device
    void interact(const DeviceInteractRef&) const final;

    // ID of the model
    ModelId model_id() const final;

    //! Name of the model, for user interaction
    std::string label() const final { return "Moller/Bhabha scattering"; }

  private:
    detail::MollerBhabhaData interface_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas
