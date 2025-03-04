//----------------------------------*-hh-*-----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RayleighInteract.hh
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "celeritas_config.h"
#include "base/Assert.hh"
#include "../detail/RayleighData.hh"

namespace celeritas
{
namespace generated
{
void rayleigh_interact(
    const detail::RayleighHostRef&,
    const ModelInteractRef<MemSpace::host>&);

void rayleigh_interact(
    const detail::RayleighDeviceRef&,
    const ModelInteractRef<MemSpace::device>&);

#if !CELERITAS_USE_CUDA
inline void rayleigh_interact(
    const detail::RayleighDeviceRef&,
    const ModelInteractRef<MemSpace::device>&)
{
    CELER_ASSERT_UNREACHABLE();
}
#endif

} // namespace generated
} // namespace celeritas
