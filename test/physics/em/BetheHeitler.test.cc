//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file BetheHeitlerInteractor.test.cc
//---------------------------------------------------------------------------//
#include "physics/em/detail/BetheHeitlerInteractor.hh"

#include "physics/material/ElementView.hh"
#include "physics/material/Types.hh"
#include "physics/base/Units.hh"
#include "physics/em/BetheHeitlerModel.hh"
#include "physics/em/GammaConversionProcess.hh"
#include "physics/material/MaterialTrackView.hh"

#include "celeritas_test.hh"
#include "gtest/Main.hh"
#include "base/ArrayUtils.hh"
#include "base/Range.hh"
#include "physics/base/Units.hh"
#include "../InteractorHostTestBase.hh"
#include "../InteractionIO.hh"

using celeritas::GammaConversionProcess;
using celeritas::detail::BetheHeitlerInteractor;
using celeritas::units::AmuMass;
namespace constants = celeritas::constants;
namespace pdg       = celeritas::pdg;

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class BetheHeitlerInteractorTest : public celeritas_test::InteractorHostTestBase
{
    using Base = celeritas_test::InteractorHostTestBase;

  protected:
    void SetUp() override
    {
        using celeritas::ParticleDef;
        using namespace celeritas::units;
        constexpr auto zero   = celeritas::zero_quantity();
        auto           stable = ParticleDef::stable_decay_constant();

        // Particles for interactor
        Base::set_particle_params(
            {{"electron",
              pdg::electron(),
              MevMass{0.5109989461},
              ElementaryCharge{-1},
              stable},
             {"positron",
              pdg::positron(),
              MevMass{0.5109989461},
              ElementaryCharge{1},
              stable},
             {"gamma", pdg::gamma(), zero, zero, stable}});
        const auto& params  = *this->particle_params();
        data_.electron_id   = params.find(pdg::electron());
        data_.positron_id   = params.find(pdg::positron());
        data_.gamma_id      = params.find(pdg::gamma());
        data_.electron_mass = params.get(data_.electron_id).mass().value();

        // Set default particle to photon with energy of 100 MeV
        this->set_inc_particle(pdg::gamma(), MevEnergy{100.0});
        this->set_inc_direction({0, 0, 1});

        // Setup MaterialView
        MaterialParams::Input inp;
        inp.elements  = {{29, AmuMass{63.546}, "Cu"}};
        inp.materials = {
            {1.0 * constants::na_avogadro,
             293.0,
             celeritas::MatterState::solid,
             {{celeritas::ElementId{0}, 1.0}},
             "Cu"},
        };
        this->set_material_params(inp);
        this->set_material("Cu");
    }

    void sanity_check(const Interaction& interaction) const
    {
        ASSERT_TRUE(interaction);

        // Check change to parent (gamma) track
        EXPECT_EQ(0, interaction.energy.value());
        EXPECT_SOFT_EQ(0, celeritas::norm(interaction.direction));
        EXPECT_EQ(celeritas::Action::absorbed, interaction.action);

        // Check secondaries
        ASSERT_EQ(2, interaction.secondaries.size());
        // Electron
        const auto& electron = interaction.secondaries.front();
        EXPECT_TRUE(electron);
        EXPECT_EQ(data_.electron_id, electron.particle_id);
        EXPECT_GT(this->particle_track().energy().value(),
                  electron.energy.value());
        EXPECT_LT(0, electron.energy.value());
        EXPECT_SOFT_EQ(1.0, celeritas::norm(electron.direction));
        // Positron
        const auto& positron = interaction.secondaries.back();
        EXPECT_TRUE(positron);
        EXPECT_EQ(data_.positron_id, positron.particle_id);
        EXPECT_GT(this->particle_track().energy().value(),
                  positron.energy.value());
        EXPECT_LT(0, positron.energy.value());
        EXPECT_SOFT_EQ(1.0, celeritas::norm(positron.direction));

        // Check conservation between primary and secondaries
        this->check_energy_conservation(interaction);
    }

  protected:
    celeritas::detail::BetheHeitlerData data_;
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(BetheHeitlerInteractorTest, basic)
{
    // Reserve 4 secondaries, two for each sample
    const int num_samples = 4;
    this->resize_secondaries(2 * num_samples);

    // Get the ElementView
    const celeritas::ElementView element(
        this->material_track().material_view().element_view(
            celeritas::ElementComponentId{0}));

    // Create the interactor
    BetheHeitlerInteractor interact(data_,
                                    this->particle_track(),
                                    this->direction(),
                                    this->secondary_allocator(),
                                    element);
    RandomEngine&          rng_engine = this->rng();
    // Produce four samples from the original/incident photon
    std::vector<double> angle;
    std::vector<double> energy1;
    std::vector<double> energy2;

    for (int i : celeritas::range(num_samples))
    {
        Interaction result = interact(rng_engine);
        SCOPED_TRACE(result);
        this->sanity_check(result);

        EXPECT_EQ(result.secondaries.data(),
                  this->secondary_allocator().get().data()
                      + result.secondaries.size() * i);

        angle.push_back(
            celeritas::dot_product(result.secondaries.front().direction,
                                   result.secondaries.back().direction));
        energy1.push_back(result.secondaries[0].energy.value());
        energy2.push_back(result.secondaries[1].energy.value());
    }

    EXPECT_EQ(2 * num_samples, this->secondary_allocator().get().size());

    // Note: these are "gold" values based on the host RNG.
    const double expected_energy1[] = {
        16.0614863783763, 98.7412722423312, 23.4953328454145, 94.7258588843146};
    const double expected_energy2[] = {
        82.9165157294237, 0.236729865468827, 75.4826692623855, 4.25214322348543};
    const double expected_angle[] = {0.999968990366521,
                                     0.749593336413488,
                                     0.999747408792083,
                                     0.99092640152178};

    EXPECT_VEC_SOFT_EQ(expected_energy1, energy1);
    EXPECT_VEC_SOFT_EQ(expected_energy2, energy2);
    EXPECT_VEC_SOFT_EQ(expected_angle, angle);

    // Next sample should fail because we're out of secondary buffer space
    {
        Interaction result = interact(rng_engine);
        EXPECT_EQ(0, result.secondaries.size());
        EXPECT_EQ(celeritas::Action::failed, result.action);
    }
}

TEST_F(BetheHeitlerInteractorTest, stress_test)
{
    const unsigned int  num_samples = 8;
    std::vector<double> avg_engine_samples;

    // Loop over a set of incident gamma energies
    for (double inc_e : {1.5, 5.0, 10.0, 50.0, 100.0})
    {
        SCOPED_TRACE("Incident energy: " + std::to_string(inc_e));
        this->set_inc_particle(pdg::gamma(), MevEnergy{inc_e});

        RandomEngine&           rng_engine            = this->rng();
        RandomEngine::size_type num_particles_sampled = 0;

        // Loop over several incident directions
        for (const Real3& inc_dir :
             {Real3{0, 0, 1}, Real3{1, 0, 0}, Real3{1e-9, 0, 1}, Real3{1, 1, 1}})
        {
            SCOPED_TRACE("Incident direction: " + to_string(inc_dir));
            this->set_inc_direction(inc_dir);
            this->resize_secondaries(2 * num_samples);

            // Get the ElementView
            const celeritas::ElementView element(
                this->material_track().material_view().element_view(
                    celeritas::ElementComponentId{0}));

            // Create interactor
            BetheHeitlerInteractor interact(data_,
                                            this->particle_track(),
                                            this->direction(),
                                            this->secondary_allocator(),
                                            element);

            // Loop over many particles
            for (unsigned int i = 0; i < num_samples; ++i)
            {
                Interaction result = interact(rng_engine);
                SCOPED_TRACE(result);
                this->sanity_check(result);
            }
            EXPECT_EQ(2 * num_samples,
                      this->secondary_allocator().get().size());
            num_particles_sampled += num_samples;
        }
        avg_engine_samples.push_back(double(rng_engine.count())
                                     / double(num_particles_sampled));
    }

    // Gold values for average number of calls to RNG
    const double expected_avg_engine_samples[]
        = {19.5, 23.5, 23.3125, 23.3125, 22.5625};
    EXPECT_VEC_SOFT_EQ(expected_avg_engine_samples, avg_engine_samples);
}
