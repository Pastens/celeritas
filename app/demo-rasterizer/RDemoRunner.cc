//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RDemoRunner.cc
//---------------------------------------------------------------------------//
#include "RDemoRunner.hh"

#include "base/CollectionStateStore.hh"
#include "base/Range.hh"
#include "base/Stopwatch.hh"
#include "base/ColorUtils.hh"
#include "comm/Logger.hh"
#include "geometry/GeoParams.hh"
#include "ImageTrackView.hh"
#include "RDemoKernel.hh"

using namespace celeritas;

namespace demo_rasterizer
{
//---------------------------------------------------------------------------//
/*!
 * Construct with image parameters
 */
RDemoRunner::RDemoRunner(SPConstGeo geometry)
    : geo_params_(std::move(geometry))
{
    CELER_EXPECT(geo_params_);
}

//---------------------------------------------------------------------------//
/*!
 * Trace an image.
 */
void RDemoRunner::operator()(ImageStore* image, int ntimes) const
{
    CELER_EXPECT(image);

    CollectionStateStore<GeoStateData, MemSpace::device> geo_state(
        *geo_params_, image->dims()[0]);

    CELER_LOG(status) << "Tracing geometry";
    // do it ntimes+1 as first one tends to be a warm-up run (slightly longer)
    double sum = 0, time = 0;
    for (int i = 0; i <= ntimes; ++i)
    {
        Stopwatch get_time;
        trace(geo_params_->device_ref(),
              geo_state.ref(),
              image->device_interface());
        time = get_time();
        CELER_LOG(info) << color_code('x') << "Elapsed " << i << ": " << time
                        << " s" << color_code(' ');
        if (i > 0)
        {
            sum += time;
        }
    }
    if (ntimes > 0)
    {
        CELER_LOG(info) << color_code('x')
                        << "\tAverage time: " << sum / ntimes << " s"
                        << color_code(' ');
    }
}

//---------------------------------------------------------------------------//
} // namespace demo_rasterizer
