//----------------------------------*-hh-*-----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file MuBremsstrahlungInteract.hh
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "celeritas_config.h"
#include "base/Assert.hh"
#include "../detail/MuBremsstrahlungData.hh"

namespace celeritas
{
namespace generated
{
void mu_bremsstrahlung_interact(
    const detail::MuBremsstrahlungHostRef&,
    const ModelInteractRef<MemSpace::host>&);

void mu_bremsstrahlung_interact(
    const detail::MuBremsstrahlungDeviceRef&,
    const ModelInteractRef<MemSpace::device>&);

#if !CELERITAS_USE_CUDA
inline void mu_bremsstrahlung_interact(
    const detail::MuBremsstrahlungDeviceRef&,
    const ModelInteractRef<MemSpace::device>&)
{
    CELER_ASSERT_UNREACHABLE();
}
#endif

} // namespace generated
} // namespace celeritas
