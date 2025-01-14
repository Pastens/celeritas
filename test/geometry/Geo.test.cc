//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file GeoTrackView.test.cc
//---------------------------------------------------------------------------//
#include "geometry/GeoParams.hh"
#include "geometry/GeoTrackView.hh"

#include "base/ArrayIO.hh"
#include "base/CollectionStateStore.hh"
#include "comm/Device.hh"
#include "comm/Logger.hh"
#include "geometry/GeoData.hh"
#include "celeritas_test.hh"

#include "GeoTestBase.hh"
#include "Geo.test.hh"

using namespace celeritas;
using namespace celeritas_test;

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

class GeoParamsHostTest : public GeoTestBase
{
  public:
    std::string filename() const override { return "fourLevels.gdml"; }
};

//---------------------------------------------------------------------------//

TEST_F(GeoParamsHostTest, accessors)
{
    const auto& geom = *this->geo_params();
    EXPECT_EQ(4, geom.num_volumes());
    EXPECT_EQ(4, geom.max_depth());

    EXPECT_EQ("Shape2", geom.id_to_label(VolumeId{0}));
    EXPECT_EQ("Shape1", geom.id_to_label(VolumeId{1}));
    EXPECT_EQ("Envelope", geom.id_to_label(VolumeId{2}));

    unsigned int nvols = geom.num_volumes();
    EXPECT_EQ("Envelope", geom.id_to_label(VolumeId{nvols - 2}));
    EXPECT_EQ("World", geom.id_to_label(VolumeId{nvols - 1}));
}

//---------------------------------------------------------------------------//

class GeoTrackViewHostTest : public GeoTestBase
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

TEST_F(GeoTrackViewHostTest, basic_tracking)
{
    const auto& geom = *this->geo_params();

    GeoTrackView geo = this->make_geo_track_view();
    geo              = {{-10, -10, -10}, {1, 0, 0}};
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2");
    EXPECT_SOFT_EQ(5, geo.next_step());

    geo.move_to_boundary(); // Shape2 -> Shape1
    EXPECT_SOFT_EQ(-5, geo.pos()[0]);
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape1");
    EXPECT_SOFT_EQ(1, geo.next_step());

    geo.move_to_boundary(); // Shape1 -> Envelope
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Envelope");
    EXPECT_FALSE(geo.is_outside());
    EXPECT_SOFT_EQ(1, geo.next_step());

    geo.move_to_boundary(); // Envelope -> World
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "World");
    EXPECT_FALSE(geo.is_outside());
}

TEST_F(GeoTrackViewHostTest, from_outside_edge)
{
    const auto& geom = *this->geo_params();

    GeoTrackView geo = this->make_geo_track_view();
    geo              = {{-24, 10., 10.}, {1, 0, 0}};
    EXPECT_TRUE(geo.is_outside());
    EXPECT_SOFT_EQ(0., geo.next_step()); // since it is on edge, but outside

    geo.move_to_boundary(); // outside -> World (still on the edge)
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "World");
    EXPECT_SOFT_EQ(7., geo.next_step()); // since it is on edge

    geo.move_to_boundary(); // World -> Envelope
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Envelope");
    EXPECT_SOFT_EQ(1., geo.next_step());

    geo.move_to_boundary(); // Envelope -> Shape1
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape1");
    EXPECT_SOFT_EQ(1., geo.next_step());

    geo.move_to_boundary(); // Shape1 -> Shape2
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2");
    EXPECT_SOFT_EQ(10., geo.next_step());
}

TEST_F(GeoTrackViewHostTest, leaving_world)
{
    const auto& geom = *this->geo_params();

    GeoTrackView geo = this->make_geo_track_view();

    geo = {{-10, 10, 10}, {0, 1, 0}};
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape2"); // Another Shape2
    EXPECT_SOFT_EQ(5.0, geo.next_step());

    geo.move_to_boundary(); // Shape2 -> Shape1
    EXPECT_SOFT_EQ(15.0, geo.pos()[1]);
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Shape1");
    EXPECT_FALSE(geo.is_outside());
    EXPECT_SOFT_EQ(1.0, geo.next_step());

    geo.move_to_boundary(); // Shape1 -> Envelope
    EXPECT_SOFT_EQ(16.0, geo.pos()[1]);
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "Envelope");
    EXPECT_FALSE(geo.is_outside());
    EXPECT_SOFT_EQ(2.0, geo.next_step());

    geo.move_to_boundary(); // Envelope -> World
    EXPECT_SOFT_EQ(18.0, geo.pos()[1]);
    EXPECT_EQ(geom.id_to_label(geo.volume_id()), "World");
    EXPECT_FALSE(geo.is_outside());
    EXPECT_SOFT_EQ(6.0, geo.next_step());

    geo.move_to_boundary(); // World -> out-of-world
    EXPECT_SOFT_EQ(24.0, geo.pos()[1]);
    EXPECT_TRUE(geo.is_outside());
    EXPECT_GT(geo.next_step(), 1.e+99);
}

//---------------------------------------------------------------------------//

#define GEO_DEVICE_TEST TEST_IF_CELERITAS_CUDA(GeoTrackViewDeviceTest)
class GEO_DEVICE_TEST : public GeoTestBase
{
  public:
    using StateStore = CollectionStateStore<GeoStateData, MemSpace::device>;

    std::string filename() const override { return "fourLevels.gdml"; }
};

TEST_F(GEO_DEVICE_TEST, all)
{
    CELER_ASSERT(this->geo_params());

    // Set up test input
    VGGTestInput input;
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
    auto output = vgg_test(input);

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
