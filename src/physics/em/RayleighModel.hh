//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RayleighModel.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/CollectionMirror.hh"
#include "physics/base/Model.hh"
#include "physics/material/MaterialParams.hh"
#include "detail/RayleighData.hh"

namespace celeritas
{
class ParticleParams;

//---------------------------------------------------------------------------//
/*!
 * Set up and launch Rayleigh scattering.
 */
class RayleighModel final : public Model
{
  public:
    //@{
    //! Type aliases
    using HostRef
        = detail::RayleighData<Ownership::const_reference, MemSpace::host>;
    using DeviceRef
        = detail::RayleighData<Ownership::const_reference, MemSpace::device>;
    //@}

  public:
    // Construct from model ID and other necessary data
    RayleighModel(ModelId               id,
                  const ParticleParams& particles,
                  const MaterialParams& materials);

    // Particle types and energy ranges that this model applies to
    SetApplicability applicability() const final;

    // Apply the interaction kernel to host data
    void interact(const HostInteractRef&) const final;

    // Apply the interaction kernel to device data
    void interact(const DeviceInteractRef&) const final;

    // ID of the model
    ModelId model_id() const final;

    //! Name of the model, for user interaction
    std::string label() const final { return "Rayleigh Scattering"; }

    //! Access Rayleigh data on the host
    const HostRef& host_ref() const { return mirror_.host(); }

    //! Access Rayleigh data on the device
    const DeviceRef& device_ref() const { return mirror_.device(); }

  private:
    //// DATA ////

    // Host/device storage and reference
    CollectionMirror<detail::RayleighData> mirror_;

    //// TYPES ////

    using AtomicNumber = int;
    using HostValue = detail::RayleighData<Ownership::value, MemSpace::host>;
    using ElScatParams = detail::RayleighParameters;

    //// HELPER FUNCTIONS ////

    void build_data(HostValue* host_data, const MaterialParams& materials);
    static const ElScatParams& get_el_parameters(AtomicNumber atomic_number);
};

//---------------------------------------------------------------------------//
} // namespace celeritas
