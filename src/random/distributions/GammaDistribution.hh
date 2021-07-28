//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file GammaDistribution.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Macros.hh"
#include "base/Types.hh"
#include "NormalDistribution.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Sample from a gamma distribution.
 *
 * The gamma distribution can be parameterized with a shape parameter \f$
 * \alpha \f$ and a scale parameter \f$ \beta \f$ and has the PDF:
 * \f[
   f(x; \alpha, \beta) = \frac{x^{\alpha - 1} e^{-x / \beta}}{\beta^\alpha
 \Gamma(\alpha)} \quad \text{ for } x > 0, \quad \alpha, \beta > 0
   \f]
 * The algorithm described in Marsaglia, G. and Tsang, W. W. "A simple method
 * for generating gamma variables". ACM Transactions on Mathematical Software.
 * 26, 3, 363–372 2000. is used here to generate gamma-distributed random
 * variables. The steps are:
 *  1. Set \f$ d = \alpha - 1/3 $ and $ c = 1 / \sqrt{9d} \f$
 *  2. Generate random variates \f$ Z \sim N(0,1) \f$ and \f$ U \sim U(0,1) \f$
 *  3. Set \f$ v = (1 + cZ)^3 \f$
 *  4. If \f$ \log U < Z^2 / 2 + d(1 - v + \log v) \f$ return \f$ dv \f$.
 *     Otherwise, go to step 2.
 * A squeeze function can be used to avoid the two logarithms in most cases by
 * accepting early if \f$ U < 1 - 0.0331 Z^4 \f$.
 *
 * Though this method is valid for \f$ \alpha \ge 1 \f$, it can easily be
 * extended for \f$ \alpha < 1 \f$: if \f$ X \sim \Gamma(\alpha + 1) \$ and \f$
 * U \sim U(0,1) \f$, then \f$ X U^{1/\alpha} \sim \Gamma(\alpha) \f$.
 */
template<class RealType = ::celeritas::real_type>
class GammaDistribution
{
  public:
    //!@{
    //! Type aliases
    using real_type   = RealType;
    using result_type = real_type;
    //!@}

  public:
    // Construct with shape and scale parameters
    explicit inline CELER_FUNCTION
    GammaDistribution(real_type alpha = 1, real_type beta = 1);

    // Sample a random number according to the distribution
    template<class Generator>
    inline CELER_FUNCTION result_type operator()(Generator& rng);

  private:
    const real_type               alpha_;
    const real_type               beta_;
    const real_type               alpha_p_;
    const real_type               d_;
    const real_type               c_;
    NormalDistribution<real_type> sample_normal_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas

#include "GammaDistribution.i.hh"