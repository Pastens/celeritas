//----------------------------------*-cu-*-----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file CombinedBremInteract.cu
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "base/Assert.hh"
#include "base/KernelParamCalculator.cuda.hh"
#include "../detail/CombinedBremLauncher.hh"

using namespace celeritas::detail;

namespace celeritas
{
namespace generated
{
namespace
{
__global__ void combined_brem_interact_kernel(
    const detail::CombinedBremDeviceRef combined_brem_data,
    const ModelInteractRef<MemSpace::device> model)
{
    auto tid = KernelParamCalculator::thread_id();
    if (!(tid < model.states.size()))
        return;

    detail::CombinedBremLauncher<MemSpace::device> launch(combined_brem_data, model);
    launch(tid);
}
} // namespace

void combined_brem_interact(
    const detail::CombinedBremDeviceRef& combined_brem_data,
    const ModelInteractRef<MemSpace::device>& model)
{
    CELER_EXPECT(combined_brem_data);
    CELER_EXPECT(model);

    static const KernelParamCalculator calc_kernel_params(
        combined_brem_interact_kernel, "combined_brem_interact");
    auto params = calc_kernel_params(model.states.size());
    combined_brem_interact_kernel<<<params.grid_size, params.block_size>>>(
        combined_brem_data, model);
    CELER_CUDA_CHECK_ERROR();
}

} // namespace generated
} // namespace celeritas
