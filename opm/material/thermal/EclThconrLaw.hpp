// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::EclThconrLaw
 */
#ifndef OPM_ECL_THCONR_LAW_HPP
#define OPM_ECL_THCONR_LAW_HPP

#include "EclThconrLawParams.hpp"

#include <opm/material/densead/Math.hpp>

namespace Opm
{
/*!
 * \ingroup material
 *
 * \brief Implements the total heat conductivity and rock enthalpy relations used by ECL.
 */
template <class ScalarT,
          class FluidSystem,
          class ParamsT = EclThconrLawParams<ScalarT> >
class EclThconrLaw
{
public:
    typedef ParamsT Params;
    typedef typename Params::Scalar Scalar;

    /*!
     * \brief Given a fluid state, return the total heat conductivity [W/m^2 / (K/m)] of the porous
     *        medium.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation heatConductivity(const Params& params,
                                       const FluidState& fluidState)
    {
        // THCONR + THCONSF approach.
        Scalar lambdaRef = params.referenceTotalHeatConductivity();
        Scalar alpha = params.dTotalHeatConductivity_dSg();

        static constexpr int gasPhaseIdx = FluidSystem::gasPhaseIdx;
        const Evaluation& Sg = Opm::decay<Evaluation>(fluidState.saturation(gasPhaseIdx));
        return lambdaRef*(1.0 - alpha*Sg);
    }
};
} // namespace Opm

#endif
