//----------------------------------*-cc-*-----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file SeltzerBergerInteract.cc
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "base/Assert.hh"
#include "base/Range.hh"
#include "base/Types.hh"
#include "../detail/SeltzerBergerLauncher.hh"

namespace celeritas
{
namespace generated
{
void seltzer_berger_interact(
    const detail::SeltzerBergerHostRef& seltzer_berger_data,
    const ModelInteractRef<MemSpace::host>& model)
{
    CELER_EXPECT(seltzer_berger_data);
    CELER_EXPECT(model);

    detail::SeltzerBergerLauncher<MemSpace::host> launch(seltzer_berger_data, model);
    #pragma omp parallel for
    for (size_type i = 0; i < model.states.size(); ++i)
    {
        ThreadId tid{i};
        launch(tid);
    }
}

} // namespace generated
} // namespace celeritas
