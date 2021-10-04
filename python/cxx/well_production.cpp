#include <tuple>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <pybind11/stl.h>
#include "export.hpp"


namespace {
    double get_oil_rate( const Well::WellProductionProperties &prop )
    {
        return prop.OilRate.get<double>();
    }

    void set_oil_rate( Well::WellProductionProperties &prop, double value )
    {
        Well::WellProductionProperties prop2 =
            const_cast<Well::WellProductionProperties&>(prop);
        prop2.OilRate.update(value);
    }

    double get_gas_rate( const Well::WellProductionProperties &prop )
    {
        return prop.GasRate.get<double>();
    }

    void set_gas_rate( Well::WellProductionProperties &prop, double value )
    {
        Well::WellProductionProperties prop2 =
            const_cast<Well::WellProductionProperties&>(prop);
        prop2.GasRate.update(value);
    }
}

void python::common::export_WellProduction(py::module& module) {

    py::class_< Well::WellProductionProperties >( module, "WellProduction")
        .def_property( "gas_rate", &get_gas_rate, &set_gas_rate )
        .def_property( "oil_rate", &get_oil_rate, &set_oil_rate );

}
