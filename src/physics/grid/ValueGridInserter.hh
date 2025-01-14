//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ValueGridInserter.hh
//---------------------------------------------------------------------------//
#pragma once

#include <vector>
#include "base/Collection.hh"
#include "base/CollectionBuilder.hh"
#include "base/Span.hh"
#include "base/Types.hh"
#include "XsGridData.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Manage data and help construction of physics value grids.
 *
 * Currently this only constructs a single value grid datatype, the
 * XsGridData, but with this framework (virtual \c
 * ValueGridXsBuilder::build method taking an instance of this class) it can be
 * extended to build additional grid types as well.
 *
 * \code
    ValueGridInserter insert(&data.host.values, &data.host.grids);
    insert(uniform_grid, values);
    store.push_back(host_ptrs);
    store.copy_to_device();
   \endcode
 */
class ValueGridInserter
{
  public:
    //!@{
    //! Type aliases
    using RealCollection
        = Collection<real_type, Ownership::value, MemSpace::host>;
    using XsGridCollection
        = Collection<XsGridData, Ownership::value, MemSpace::host>;
    using SpanConstReal    = Span<const real_type>;
    using InterpolatedGrid = std::pair<SpanConstReal, Interp>;
    using XsIndex          = ItemId<XsGridData>;
    using GenericIndex     = ItemId<GenericGridData>;
    //!@}

  public:
    // Construct with a reference to mutable host data
    ValueGridInserter(RealCollection* real_data, XsGridCollection* xs_grid);

    // Add a grid of xs-like data
    XsIndex operator()(const UniformGridData& log_grid,
                       size_type              prime_index,
                       SpanConstReal          values);

    // Add a grid of uniform log-grid data
    XsIndex operator()(const UniformGridData& log_grid, SpanConstReal values);

    // Add a grid of generic data
    GenericIndex operator()(InterpolatedGrid grid, InterpolatedGrid values);

  private:
    CollectionBuilder<real_type, MemSpace::host, ItemId<real_type>>   values_;
    CollectionBuilder<XsGridData, MemSpace::host, ItemId<XsGridData>> xs_grids_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas
