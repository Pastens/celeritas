//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file VectorUtils.hh
//! \brief Helper functions for std::vector
//---------------------------------------------------------------------------//
#pragma once

#include <vector>
#include "base/Types.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
// Return evenly spaced numbers over a specific interval
std::vector<real_type> linspace(real_type start, real_type stop, size_type n);

//---------------------------------------------------------------------------//
} // namespace celeritas
