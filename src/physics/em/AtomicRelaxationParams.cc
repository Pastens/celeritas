//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file AtomicRelaxationParams.cc
//---------------------------------------------------------------------------//
#include "AtomicRelaxationParams.hh"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <unordered_map>
#include <vector>
#include "base/Range.hh"
#include "base/SoftEqual.hh"
#include "detail/Utils.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct from a vector of element identifiers.
 *
 * \note The EADL only provides transition probabilities for 6 <= Z <= 100, so
 * there will be no atomic relaxation data for Z < 6. Transitions are only
 * provided for K, L, M, N, and some O shells.
 */
AtomicRelaxationParams::AtomicRelaxationParams(const Input& inp)
    : is_auger_enabled_(inp.is_auger_enabled)
{
    CELER_EXPECT(inp.cutoffs);
    CELER_EXPECT(inp.materials);
    CELER_EXPECT(inp.particles);

    HostData host_data;

    // Get particle IDs
    host_data.electron_id = inp.particles->find(pdg::electron());
    host_data.gamma_id    = inp.particles->find(pdg::gamma());
    CELER_VALIDATE(host_data.electron_id && host_data.gamma_id,
                   << "missing electron and/or gamma particles "
                      "(required for atomic relaxation)");

    // Find the minimum electron and photon cutoff energy for each element over
    // all materials. This is used to calculate the maximum number of
    // secondaries that could be created in atomic relaxation for each element.
    size_type              num_elements = inp.materials->num_elements();
    std::vector<MevEnergy> electron_cutoff(num_elements, max_quantity());
    std::vector<MevEnergy> gamma_cutoff(num_elements, max_quantity());
    for (auto mat_id : range(MaterialId{inp.materials->num_materials()}))
    {
        // Electron and photon energy cutoffs for this material
        auto cutoffs  = inp.cutoffs->get(mat_id);
        auto material = inp.materials->get(mat_id);
        for (auto comp_id : range(ElementComponentId{material.num_elements()}))
        {
            auto el_idx             = material.element_id(comp_id).get();
            electron_cutoff[el_idx] = min(
                electron_cutoff[el_idx], cutoffs.energy(host_data.electron_id));
            gamma_cutoff[el_idx] = min(gamma_cutoff[el_idx],
                                       cutoffs.energy(host_data.gamma_id));
        }
    }

    // Build elements
    make_builder(&host_data.elements).reserve(num_elements);
    for (auto el_idx : range(num_elements))
    {
        AtomicNumber z = inp.materials->get(ElementId{el_idx}).atomic_number();
        this->append_element(inp.load_data(z),
                             &host_data,
                             electron_cutoff[el_idx],
                             gamma_cutoff[el_idx]);
    }

    // Move to mirrored data, copying to device
    data_ = CollectionMirror<AtomicRelaxParamsData>{std::move(host_data)};
    CELER_ENSURE(data_);
}

//---------------------------------------------------------------------------//
// IMPLEMENTATION
//---------------------------------------------------------------------------//
/*!
 * Convert an element input to a AtomicRelaxElement and store.
 */
void AtomicRelaxationParams::append_element(const ImportAtomicRelaxation& inp,
                                            HostData*                     data,
                                            MevEnergy electron_cutoff,
                                            MevEnergy gamma_cutoff)
{
    CELER_EXPECT(data);
    AtomicRelaxElement el;

    // Collect all the subshell designators for this element
    std::set<int> designators;
    for (const auto& shell : inp.shells)
    {
        designators.insert(shell.designator);

        // Check that for a given subshell vacancy EADL transition
        // probabilities are normalized so that the sum over all radiative and
        // non-radiative transitions is 1
        real_type norm = 0.;
        for (const auto& transition : shell.fluor)
        {
            norm += transition.probability;
            designators.insert(transition.initial_shell);
        }
        for (const auto& transition : shell.auger)
        {
            norm += transition.probability;
            designators.insert(transition.initial_shell);
            designators.insert(transition.auger_shell);
        }
        CELER_ASSERT(soft_equal(1., norm));
    }

    // Create a mapping of subshell designator to index in the shells array (it
    // is ok for an index to be greater than or equal to the size of the shells
    // array; this just means there is no transition data for that shell)
    std::unordered_map<int, SubshellId> des_to_id;
    size_type                           index = 0;
    for (auto des : designators)
    {
        des_to_id[des] = SubshellId{index++};
    }
    CELER_ASSERT(des_to_id.size() >= inp.shells.size());

    // Add subshell data
    std::vector<AtomicRelaxSubshell> shells(inp.shells.size());
    for (auto i : range(inp.shells.size()))
    {
        // Get all the transitions for this subshell
        std::vector<ImportAtomicTransition> import_transitions(
            inp.shells[i].fluor.begin(), inp.shells[i].fluor.end());
        if (is_auger_enabled_)
        {
            // Append the non-radiative transitions if Auger effect is enabled
            import_transitions.insert(import_transitions.end(),
                                      inp.shells[i].auger.begin(),
                                      inp.shells[i].auger.end());
        }

        // Add transition data
        std::vector<AtomicRelaxTransition> transitions(
            import_transitions.size());
        for (auto j : range(import_transitions.size()))
        {
            // Find the index in the shells array given the shell designator.
            // If the designator is not found, map it to an invalid value.
            transitions[j].initial_shell
                = des_to_id[import_transitions[j].initial_shell];
            transitions[j].auger_shell
                = des_to_id[import_transitions[j].auger_shell];
            transitions[j].probability = import_transitions[j].probability;
            transitions[j].energy      = import_transitions[j].energy;
        }
        shells[i].transitions
            = make_builder(&data->transitions)
                  .insert_back(transitions.begin(), transitions.end());
    }
    el.shells
        = make_builder(&data->shells).insert_back(shells.begin(), shells.end());

    // Calculate the maximum possible number of secondaries that could be
    // created in atomic relaxation.
    el.max_secondary = detail::calc_max_secondaries(
        make_const_ref(*data), el.shells, electron_cutoff, gamma_cutoff);

    // Maximum size of the stack used to store unprocessed vacancy subshell IDs
    data->max_stack_size
        = max(data->max_stack_size,
              detail::calc_max_stack_size(make_const_ref(*data), el.shells));

    // Add the elemental data (no data for Z < 6)
    CELER_ASSERT(el || inp.shells.empty());
    make_builder(&data->elements).push_back(el);
}

//---------------------------------------------------------------------------//
} // namespace celeritas
