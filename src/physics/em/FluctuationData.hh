//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file FluctuationData.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Macros.hh"
#include "base/Types.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Material-dependent parameters used in the energy loss fluctuation model.
 */
struct FluctuationParameters
{
    using Real2 = Array<real_type, 2>;

    Real2 binding_energy;      //!< Binding energies E_1 and E_2 [MeV]
    Real2 log_binding_energy;  //!< Log of binding energies
    Real2 oscillator_strength; //!< Oscillator strengths f_1 and f_2
};

//---------------------------------------------------------------------------//
/*!
 * Data needed to sample from the energy loss distribution.
 */
template<Ownership W, MemSpace M>
struct FluctuationData
{
    template<class T>
    using MaterialItems = Collection<T, W, M, MaterialId>;

    //// MEMBER DATA ////

    ParticleId electron_id;                      //!< ID of an electron
    real_type  electron_mass;                    //!< Electron mass [MevMass]
    MaterialItems<FluctuationParameters> params; //!< Model parameters

    //// MEMBER FUNCTIONS ////

    //! Check whether the data is assigned
    explicit CELER_FUNCTION operator bool() const
    {
        return electron_id && electron_mass > 0;
    }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    FluctuationData& operator=(const FluctuationData<W2, M2>& other)
    {
        electron_id   = other.electron_id;
        electron_mass = other.electron_mass;
        params        = other.params;
        return *this;
    }
};

//---------------------------------------------------------------------------//
} // namespace celeritas
