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
* \copydoc Opm::H2GasPvt
*/
#ifndef OPM_H2_GAS_PVT_HPP
#define OPM_H2_GAS_PVT_HPP

#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/H2.hpp>
#include <opm/material/binarycoefficients/Brine_H2.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

#include <vector>

namespace Opm {

/*!
* \brief This class represents the Pressure-Volume-Temperature relations of the gas phase for H2
*/
template <class Scalar>
class H2GasPvt
{
    typedef SimpleHuDuanH2O<Scalar> H2O;
    typedef ::Opm::H2<Scalar> H2;

public:
    // The binary coefficients for brine and H2 used by this fluid system
    typedef BinaryCoeff::Brine_H2<Scalar, H2O, H2> BinaryCoeffBrineH2;

    explicit H2GasPvt() = default;
    H2GasPvt(const std::vector<Scalar>& gasReferenceDensity)
        : gasReferenceDensity_(gasReferenceDensity)
    {
    }

#if HAVE_ECL_INPUT
    /*!
    * \brief Initialize the parameters for H2 gas using an ECL deck.
    */
    void initFromState(const EclipseState& eclState, const Schedule&)
    {
        if( !eclState.getTableManager().getDensityTable().empty()) {
            std::cerr << "WARNING: H2STORE is enabled but DENSITY is in the deck. \n" <<
                         "The surface density is computed based on H2-BRINE PVT at standard conditions (STCOND)" <<
                         " and DENSITY is ignored " << std::endl;
        }

        if( eclState.getTableManager().hasTables("PVDG") || !eclState.getTableManager().getPvtgTables().empty()) {
            std::cerr << "WARNING: H2STORE is enabled but PVDG or PVTG is in the deck. \n" <<
                         "H2 pvt properties are calculated based on ideal gas relations, and PVDG/PVTG input" <<
                         " is ignored. " << std::endl;
        }

        // We only supported single pvt region for the H2-brine module
        size_t numRegions = 1;
        setNumRegions(numRegions);
        size_t regionIdx = 0;
        Scalar T_ref = eclState.getTableManager().stCond().temperature;
        Scalar P_ref = eclState.getTableManager().stCond().pressure;
        gasReferenceDensity_[regionIdx] = H2::gasDensity(T_ref, P_ref);
        initEnd();
    }
#endif

    void setNumRegions(size_t numRegions)
    {
        gasReferenceDensity_.resize(numRegions);
    }


    /*!
    * \brief Initialize the reference densities of all fluids for a given PVT region
    */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar /*rhoRefOil*/,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/)
    {
        gasReferenceDensity_[regionIdx] = rhoRefGas;
    }

    /*!
    * \brief Finish initializing the oil phase PVT properties.
    */
    void initEnd()
    {

    }

    /*!
    * \brief Return the number of PVT regions which are considered by this PVT-object.
    */
    unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    /*!
    * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
    */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation&) const
    {
        return H2::gasInternalEnergy(temperature, pressure);
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rv*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        return H2::gasViscosity(temperature, pressure);
    }

    /*!
    * \brief Returns the formation volume factor [-] of the fluid phase.
    */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& /*Rv*/) const
    { return saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure); }

    /*!
    * \brief Returns the formation volume factor [-] of oil saturated gas at given pressure.
    */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        return H2::gasDensity(temperature, pressure)/gasReferenceDensity_[regionIdx];
    }

    /*!
    * \brief Returns the saturation pressure of the gas phase [Pa] depending on its mass fraction of the oil component
    *
    * \param Rv The surface volume of oil component dissolved in what will yield one cubic meter of gas at the surface [-]
    */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rv*/) const
    { return 0.0; /* this is dry gas! */ }

    /*!
    * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
    */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/,
                                              const Evaluation& /*oilSaturation*/,
                                              const Evaluation& /*maxOilSaturation*/) const
    { return 0.0; /* this is dry gas! */ }

    /*!
    * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
    */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dry gas! */ }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        return BinaryCoeffBrineH2::gasDiffCoeff(temperature, pressure);
    }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    bool operator==(const H2GasPvt<Scalar>& data) const
    {
        return gasReferenceDensity_ == data.gasReferenceDensity_;
    }

private:
    std::vector<Scalar> gasReferenceDensity_;
};  // end class H2GasPvt

}  // end namspace Opm

#endif