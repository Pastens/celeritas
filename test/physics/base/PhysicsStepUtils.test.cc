//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file PhysicsStepUtils.test.cc
//---------------------------------------------------------------------------//
#include "physics/base/PhysicsStepUtils.hh"

#include "base/CollectionStateStore.hh"
#include "random/DiagnosticRngEngine.hh"
#include "physics/base/CutoffParams.hh"
#include "physics/base/ParticleParams.hh"
#include "physics/base/PhysicsParams.hh"
#include "celeritas_test.hh"
#include "PhysicsTestBase.hh"

using namespace celeritas;
using namespace celeritas_test;
using celeritas::units::MevEnergy;

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class PhysicsStepUtilsTest : public PhysicsTestBase
{
    using Base         = PhysicsTestBase;
    using RandomEngine = DiagnosticRngEngine<std::mt19937>;

  protected:
    using MaterialStateStore
        = CollectionStateStore<MaterialStateData, MemSpace::host>;
    using ParticleStateStore
        = CollectionStateStore<ParticleStateData, MemSpace::host>;
    using PhysicsStateStore
        = CollectionStateStore<PhysicsStateData, MemSpace::host>;

    PhysicsOptions build_physics_options() const override
    {
        PhysicsOptions opts;
        if (/* DISABLES CODE */ (false))
        {
            // Don't scale the range -- use exactly the analytic values our
            // model has.
            opts.min_range = inf;
        }
        return opts;
    }

    void SetUp() override
    {
        Base::SetUp();

        // Construct state for a single host thread
        mat_state  = MaterialStateStore(*this->materials(), 1);
        par_state  = ParticleStateStore(*this->particles(), 1);
        phys_state = PhysicsStateStore(*this->physics(), 1);
    }

    //!@{
    //! Random number generator
    RandomEngine& rng() { return rng_; }
    //!@}

    PhysicsTrackView init_track(MaterialTrackView* mat,
                                MaterialId         mid,
                                ParticleTrackView* par,
                                const char*        name,
                                MevEnergy          energy)
    {
        CELER_EXPECT(mat && par);
        CELER_EXPECT(mid < this->materials()->size());
        *mat = MaterialTrackView::Initializer_t{mid};

        ParticleTrackView::Initializer_t par_init;
        par_init.particle_id = this->particles()->find(name);
        CELER_EXPECT(par_init.particle_id);
        par_init.energy = energy;
        *par            = par_init;

        PhysicsTrackView phys(this->physics()->host_ref(),
                              phys_state.ref(),
                              par->particle_id(),
                              mat->material_id(),
                              ThreadId{0});
        return phys;
    }

    MaterialStateStore mat_state;
    ParticleStateStore par_state;
    PhysicsStateStore  phys_state;
    RandomEngine       rng_;
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(PhysicsStepUtilsTest, calc_tabulated_physics_step)
{
    MaterialTrackView material(
        this->materials()->host_ref(), mat_state.ref(), ThreadId{0});
    ParticleTrackView particle(
        this->particles()->host_ref(), par_state.ref(), ThreadId{0});

    // Test a variety of energies and multiple material IDs
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{0}, &particle, "gamma", MevEnergy{1});
        phys.interaction_mfp(1);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1. / 3.e-4, step);
    }
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{1}, &particle, "celeriton", MevEnergy{10});
        phys.interaction_mfp(1e-4);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1.e-4 / 9.e-3, step);

        // Increase the distance to interaction so range limits the step length
        phys.interaction_mfp(1);
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(0.48853333333333326, step);
    }
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{1}, &particle, "celeriton", MevEnergy{1e-2});
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(0.0016666666666666663, step);
    }
    {
        PhysicsTrackView phys = this->init_track(&material,
                                                 MaterialId{2},
                                                 &particle,
                                                 "anti-celeriton",
                                                 MevEnergy{1e-2});
        phys.interaction_mfp(1e-6);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1.e-6 / 9.e-1, step);

        // Increase the distance to interaction so range limits the step length
        phys.interaction_mfp(1);
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1.4285714285714282e-5, step);
    }
    {
        PhysicsTrackView phys = this->init_track(&material,
                                                 MaterialId{2},
                                                 &particle,
                                                 "anti-celeriton",
                                                 MevEnergy{10});
        real_type        step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(0.014285714285714284, step);
    }
}

TEST_F(PhysicsStepUtilsTest, calc_energy_loss)
{
    MaterialTrackView material(
        this->materials()->host_ref(), mat_state.ref(), ThreadId{0});
    ParticleTrackView particle(
        this->particles()->host_ref(), par_state.ref(), ThreadId{0});

    // Construct empty cutoff params. If the cutoff energy is zero, no
    // fluctuations will be added to the mean loss
    CutoffParams::Input cutoff_input{this->particles(), this->materials(), {}};
    CutoffParams        cutoff_params(cutoff_input);
    CutoffView          cutoffs(cutoff_params.host_ref(), MaterialId{0});

    {
        // Long step, but gamma means no energy loss
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{0}, &particle, "gamma", MevEnergy{1});
        EXPECT_SOFT_EQ(0,
                       celeritas::calc_energy_loss(
                           cutoffs, material, particle, phys, 1e4, this->rng())
                           .value());
    }
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{0}, &particle, "celeriton", MevEnergy{10});
        const real_type eloss_rate = 0.2 + 0.4;

        // Tiny step: should still be linear loss (single process)
        EXPECT_SOFT_EQ(eloss_rate * 1e-6,
                       celeritas::calc_energy_loss(
                           cutoffs, material, particle, phys, 1e-6, this->rng())
                           .value());

        // Long step (lose half energy) will call inverse lookup. The correct
        // answer (if range table construction was done over energy loss)
        // should be half since the slowing down rate is constant over all
        real_type step = 0.5 * particle.energy().value() / eloss_rate;
        EXPECT_SOFT_EQ(5,
                       celeritas::calc_energy_loss(
                           cutoffs, material, particle, phys, step, this->rng())
                           .value());

        // Long step (lose half energy) will call inverse lookup. The correct
        // answer (if range table construction was done over energy loss)
        // should be half since the slowing down rate is constant over all
        step = 0.999 * particle.energy().value() / eloss_rate;
        EXPECT_SOFT_EQ(9.99,
                       celeritas::calc_energy_loss(
                           cutoffs, material, particle, phys, step, this->rng())
                           .value());
    }
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{0}, &particle, "electron", MevEnergy{1e-3});
        const real_type eloss_rate = 0.5;

        // Low energy particle which loses all its energy over the step will
        // call inverse lookup. Remaining range will be zero and eloss will be
        // equal to the pre-step energy.
        const real_type step = particle.energy().value() / eloss_rate;
        EXPECT_SOFT_EQ(1e-3,
                       celeritas::calc_energy_loss(
                           cutoffs, material, particle, phys, step, this->rng())
                           .value());
    }
}

TEST_F(PhysicsStepUtilsTest, select_process_and_model)
{
    MaterialTrackView material(
        this->materials()->host_ref(), mat_state.ref(), ThreadId{0});
    ParticleTrackView particle(
        this->particles()->host_ref(), par_state.ref(), ThreadId{0});

    unsigned int num_samples = 0;

    // Test a variety of energy ranges and multiple material IDs
    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{0}, &particle, "gamma", MevEnergy{1});
        phys.interaction_mfp(1);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1. / 3.e-4, step);

        // Testing cheat.
        PhysicsTrackView::PhysicsStateRef state_shortcut(phys_state.ref());
        state_shortcut.state[ThreadId{0}].interaction_mfp = 0;

        auto result = select_process_and_model(particle, phys, this->rng());
        EXPECT_EQ(result.ppid.get(), 0);
        EXPECT_EQ(result.model.get(), 0);
        ++num_samples;

        result = select_process_and_model(particle, phys, this->rng());
        EXPECT_EQ(result.ppid.get(), 1);
        EXPECT_EQ(result.model.get(), 2);
        ++num_samples;

        result = select_process_and_model(particle, phys, this->rng());
        EXPECT_EQ(result.ppid.get(), 1);
        EXPECT_EQ(result.model.get(), 2);
        ++num_samples;
    }

    {
        PhysicsTrackView phys = this->init_track(
            &material, MaterialId{1}, &particle, "celeriton", MevEnergy{10});
        phys.interaction_mfp(1);

        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(0.48853333333333326, step);

        // Testing cheat.
        PhysicsTrackView::PhysicsStateRef state_shortcut(phys_state.ref());
        state_shortcut.state[ThreadId{0}].interaction_mfp = 0;

        // The expected values are correlated with the values generated by the
        // random number generator and by the energy of particle.
        // The number of tries is picked so that each process is selected at
        // least once.
        using restype = std::pair<unsigned int, unsigned int>;
        std::vector<restype> expected({{1, 5},
                                       {2, 8},
                                       {2, 8},
                                       {2, 8},
                                       {2, 8},
                                       {2, 8},
                                       {0, 1},
                                       {1, 5},
                                       {2, 8},
                                       {2, 8},
                                       {1, 5},
                                       {1, 5},
                                       {2, 8}});
        std::vector<restype> results; // Could add:
                                      // result.reserve(expected.size());
        for (auto i : range(expected.size()))
        {
            (void)i;
            auto result = select_process_and_model(particle, phys, this->rng());
            results.emplace_back(result.ppid.get(), result.model.get());
            ++num_samples;
        }
        EXPECT_VEC_EQ(results, expected);
        EXPECT_EQ(56, this->rng().count());
    }
    {
        // Test the integral approach
        unsigned int           num_samples   = 10000;
        std::vector<real_type> inc_energy    = {0.01, 0.01, 0.1, 10};
        std::vector<real_type> scaled_energy = {0.001, 0.00999, 0.001, 8};
        std::vector<real_type> acceptance_rate;

        for (auto i : range(inc_energy.size()))
        {
            PhysicsTrackView  phys = this->init_track(&material,
                                                     MaterialId{0},
                                                     &particle,
                                                     "electron",
                                                     MevEnergy{inc_energy[i]});
            ParticleProcessId ppid{0};
            EXPECT_TRUE(phys.use_integral_xs(ppid));
            auto grid_id = phys.value_grid(ValueGridType::macro_xs, ppid);
            CELER_ASSERT(grid_id);

            // Get the estimate of the maximum cross section over the step
            real_type xs_max = phys.calc_xs(ppid, grid_id, particle.energy());
            phys.per_process_xs(ppid) = xs_max;

            // Set the post-step energy
            particle.energy(MevEnergy{scaled_energy[i]});

            unsigned int count = 0;
            for (unsigned int j = 0; j < num_samples; ++j)
            {
                phys.interaction_mfp(0);
                phys.macro_xs(xs_max);

                if (select_process_and_model(particle, phys, this->rng()))
                    ++count;
            }
            acceptance_rate.push_back(real_type(count) / num_samples);
        }
        const real_type expected_acceptance_rate[]
            = {0.9204, 0.9999, 0.4972, 1};
        EXPECT_VEC_EQ(expected_acceptance_rate, acceptance_rate);
    }
}
