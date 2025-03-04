//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file LinearPropagator.test.cc
//---------------------------------------------------------------------------//
#include "geometry/LinearPropagator.hh"

#include "base/ArrayIO.hh"
#include "base/CollectionStateStore.hh"
#include "comm/Device.hh"
#include "comm/Logger.hh"
#include "geometry/GeoParams.hh"
#include "geometry/GeoData.hh"
#include "celeritas_test.hh"

#include "GeoTestBase.hh"
#include "LinearPropagator.test.hh"

using namespace celeritas;
using namespace celeritas_test;

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class LinearPropagatorHostTest : public GeoTestBase
{
  public:
    using StateStore = CollectionStateStore<GeoStateData, MemSpace::host>;

    std::string filename() const override { return "fourLevels.gdml"; }

    void SetUp() override { state = StateStore(*this->geo_params(), 1); }

    GeoTrackView make_geo_track_view()
    {
        return GeoTrackView(
            this->geo_params()->host_ref(), state.ref(), ThreadId(0));
    }

  protected:
    StateStore state;
};

//---------------------------------------------------------------------------//
// HOST TESTS
//---------------------------------------------------------------------------//

TEST_F(LinearPropagatorHostTest, accessors)
{
    const auto& geom = *this->geo_params();
    EXPECT_EQ(4, geom.num_volumes());
    EXPECT_EQ(4, geom.max_depth());

    EXPECT_EQ("Shape2", geom.id_to_label(VolumeId{0}));
    EXPECT_EQ("Shape1", geom.id_to_label(VolumeId{1}));
}

//----------------------------------------------------------------------------//

TEST_F(LinearPropagatorHostTest, basic_tracking)
{
    GeoTrackView     geo = this->make_geo_track_view();
    LinearPropagator propagate(&geo); // one propagator per track

    const auto& geom = *this->geo_params();
    {
        // Track from outside detector, moving right
        geo = {{-10, 10, 10}, {1, 0, 0}};
        EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2"); // in Shape2

        auto step = propagate(1.e10); // very large proposed step
        EXPECT_SOFT_EQ(5, step.distance);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1");
        EXPECT_SOFT_EQ(-5, geo.pos()[0]);

        step = propagate(1.e10);
        EXPECT_SOFT_EQ(1, step.distance);
        EXPECT_EQ(geom.id_to_label(step.volume), "Envelope");
        EXPECT_FALSE(geo.is_outside());

        step = propagate();
        EXPECT_SOFT_EQ(1, step.distance);
        EXPECT_FALSE(geo.is_outside());
    }

    {
        // Track from outside edge used to fail
        CELER_LOG(info) << "Init a track just outside of world volume...";
        geo = {{-24, 6.5, 6.5}, {1, 0, 0}};
        EXPECT_TRUE(geo.is_outside());
        EXPECT_SOFT_EQ(0., geo.next_step()); // since on edge, but outside

        auto step = propagate(); // outside -> World
        EXPECT_FALSE(geo.is_outside());
        EXPECT_EQ(geom.id_to_label(step.volume), "World");
        EXPECT_SOFT_EQ(7., geo.next_step());

        step = propagate(); // World -> Envelope
        EXPECT_EQ(geom.id_to_label(step.volume), "Envelope");
        EXPECT_SOFT_EQ(7., step.distance);
        EXPECT_SOFT_EQ(1., geo.next_step());

        step = propagate(); // Envelope -> Shape1
        EXPECT_SOFT_EQ(1., step.distance);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1");

        step = propagate();                                 // Shape1 -> Shape2
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape2"); // bad
    }

    {
        // Track from inside detector
        geo = {{-10, 10, 10}, {0, 1, 0}};
        EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2"); // Shape2
        EXPECT_SOFT_EQ(5.0, geo.next_step());

        auto step = propagate(); // Shape2 -> Shape1
        EXPECT_SOFT_EQ(5.0, step.distance);
        EXPECT_SOFT_EQ(15.0, geo.pos()[1]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1");
        EXPECT_FALSE(geo.is_outside());
        EXPECT_SOFT_EQ(1.0, geo.next_step());

        step = propagate(); // Shape1 -> Envelope
        EXPECT_SOFT_EQ(16.0, geo.pos()[1]);
        EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Envelope");
        EXPECT_FALSE(geo.is_outside());
        EXPECT_SOFT_EQ(2.0, geo.next_step());

        step = propagate(); // Envelope -> World
        EXPECT_SOFT_EQ(18.0, geo.pos()[1]);
        EXPECT_EQ(geom.id_to_label(geo.volume_id()), "World");
        EXPECT_FALSE(geo.is_outside());
        EXPECT_SOFT_EQ(6.0, geo.next_step());

        step = propagate(); // World -> out-of-world
        EXPECT_SOFT_EQ(24.0, geo.pos()[1]);
        EXPECT_TRUE(geo.is_outside());
        EXPECT_GT(geo.next_step(), 1.e+99);
    }
}

//----------------------------------------------------------------------------//

TEST_F(LinearPropagatorHostTest, track_intraVolume)
{
    GeoTrackView     geo = this->make_geo_track_view();
    LinearPropagator propagate(&geo); // one propagator per track

    const auto& geom = *this->geo_params();
    {
        // Track from outside detector, moving right
        geo = {{-10, 10, 10}, {0, 0, 1}};
        EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2"); // Shape2

        // break next step into two
        auto step = propagate(0.5 * geo.next_step());
        EXPECT_SOFT_EQ(2.5, step.distance);
        EXPECT_SOFT_EQ(12.5, geo.pos()[2]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape2"); // still Shape2

        step = propagate(); // all remaining
        EXPECT_SOFT_EQ(2.5, step.distance);
        EXPECT_SOFT_EQ(15.0, geo.pos()[2]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1"); // Shape2 -> Shape1

        // break next step into > 2 steps, re-calculating next_step each time
        step = propagate(0.2 * geo.next_step()); // step 1 inside Shape1
        EXPECT_SOFT_EQ(0.2, step.distance);
        EXPECT_SOFT_EQ(15.2, geo.pos()[2]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1");

        EXPECT_SOFT_EQ(0.8, geo.next_step());

        step = propagate(0.5 * geo.next_step()); // step 2 inside Shape1
        EXPECT_SOFT_EQ(0.4, step.distance);
        EXPECT_SOFT_EQ(15.6, geo.pos()[2]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Shape1");

        EXPECT_SOFT_EQ(0.4, geo.next_step());

        step = propagate(geo.next_step()); // last step inside Shape1
        EXPECT_SOFT_EQ(0.4, step.distance);
        EXPECT_SOFT_EQ(16, geo.pos()[2]);
        EXPECT_EQ(geom.id_to_label(step.volume), "Envelope");
    }
}

//---------------------------------------------------------------------------//
// DEVICE TESTS
//---------------------------------------------------------------------------//

#define LP_DEVICE_TEST TEST_IF_CELERITAS_CUDA(LinearPropagatorDeviceTest)
class LP_DEVICE_TEST : public GeoTestBase
{
  public:
    using StateStore = CollectionStateStore<GeoStateData, MemSpace::device>;

    std::string filename() const override { return "fourLevels.gdml"; }
};

TEST_F(LP_DEVICE_TEST, track_lines)
{
    CELER_ASSERT(this->geo_params());

    // Set up test input
    LinPropTestInput input;
    input.init = {{{10, 10, 10}, {1, 0, 0}},
                  {{10, 10, -10}, {1, 0, 0}},
                  {{10, -10, 10}, {1, 0, 0}},
                  {{10, -10, -10}, {1, 0, 0}},
                  {{-10, 10, 10}, {-1, 0, 0}},
                  {{-10, 10, -10}, {-1, 0, 0}},
                  {{-10, -10, 10}, {-1, 0, 0}},
                  {{-10, -10, -10}, {-1, 0, 0}}};
    StateStore device_states(*this->geo_params(), input.init.size());

    input.max_segments = 3;
    input.params       = this->geo_params()->device_ref();
    input.state        = device_states.ref();

    // Run kernel
    auto output = linprop_test(input);

    // clang-format off
    static const int expected_ids[] = {
        1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3,
        1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};

    static const double expected_distances[]
        = {5, 1, 1, 5, 1, 1, 5, 1, 1, 5, 1, 1,
           5, 1, 1, 5, 1, 1, 5, 1, 1, 5, 1, 1};
    // clang-format on

    // Check results
    EXPECT_VEC_EQ(expected_ids, output.ids);
    EXPECT_VEC_SOFT_EQ(output.distances, expected_distances);
}
