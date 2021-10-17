#include <ctime>
#include <chrono>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include "export.hpp"


namespace {

    using system_clock = std::chrono::system_clock;


    /*
      timezones - the stuff that make you wonder why didn't do social science in
      university. The situation here is as follows:

      1. In the C++ code Eclipse style string literals like "20. NOV 2017" are
         converted to time_t values using the utc based function timegm() which
         does not take timezones into account.

      2. Here we use the function gmtime( ) to convert back from a time_t value
         to a broken down struct tm representation.

      3. The broken down representation is then converted to a time_t value
         using the timezone aware function mktime().

      4. The time_t value is converted to a std::chrono::system_clock value.

      Finally std::chrono::system_clock value is automatically converted to a
      python datetime object as part of the pybind11 process. This latter
      conversion *is* timezone aware, that is the reason we must go through
      these hoops.
    */
    system_clock::time_point datetime( std::time_t utc_time) {
        struct tm utc_tm;
        time_t local_time;

        gmtime_r(&utc_time, &utc_tm);
        local_time = mktime(&utc_tm);

        return system_clock::from_time_t(local_time);
    }

    const Well& get_well( const Schedule& sch, const std::string& name, const size_t& timestep ) try {
        return sch.getWell( name, timestep );
    } catch( const std::invalid_argument& e ) {
        throw py::key_error( name );
    }

    system_clock::time_point get_start_time( const Schedule& s ) {
        return datetime(s.posixStartTime());
    }

    system_clock::time_point get_end_time( const Schedule& s ) {
        return datetime(s.posixEndTime());
    }

    std::vector<system_clock::time_point> get_timesteps( const Schedule& s ) {
        std::vector< system_clock::time_point > v;

        for( size_t i = 0; i < s.size(); ++i )
            v.push_back( datetime( std::chrono::system_clock::to_time_t(s[i].start_time() )));

        return v;
    }

    std::vector<Group> get_groups( const Schedule& sch, size_t timestep ) {
        std::vector< Group > groups;
        for( const auto& group_name : sch.groupNames())
            groups.push_back( sch.getGroup(group_name, timestep) );

        return groups;
    }

    bool has_well( const Schedule& sch, const std::string& wellName) {
        return sch.hasWell( wellName );
    }

    const Group& get_group(const ScheduleState& st, const std::string& group_name) {
        return st.groups.get(group_name);
    }


    const ScheduleState& getitem(const Schedule& sch, std::size_t index) {
        return sch[index];
    }

    double get_well_production_target(
        const Schedule& sch,
        const std::string& well_name,
        std::size_t index,
        const std::string& variable)
    {
        const Well& well = get_well(sch, well_name, index);
        const Well::WellProductionProperties& prop = well.getProductionProperties();
        if (variable == "oil") {
            return prop.OilRate.get<double>();
        }
        else if (variable == "gas") {
            return prop.GasRate.get<double>();
        }
        else {
            throw py::value_error("Unknown variable: " + variable);
        }
    }

    void set_well_production_target(
        Schedule& sch,
        const std::string& well_name,
        std::size_t index,
        const std::string& variable,
        double value
    )
    {
        // NOTE: We are going to modify the well's production properties, so we
        //   cannot reuse the shared pointer (since it might be shared with some
        //   other well's production properties which should not be modified)
        //  Unfortunately, this also lead to a change in the Opm::Well itself
        //   since it has the WellProductionProperties as a class variable, and
        //   is itself a shared pointer that can be reused at different report steps.
        //  This means that we need to create a new Opm::Well object also.
        //
        const Well& old_well = get_well(sch, well_name, index);

        //  Create a copy of the old well structure, the update() method in
        //  ScheduleState will later create a shared_ptr from it.
        Well well = old_well;
        //  Create a new shared pointer by copy constructing the reference
        //  to the old one
        std::shared_ptr<Well::WellProductionProperties> prop
            = std::make_shared<Well::WellProductionProperties>(
                  well.getProductionProperties());
        if (variable == "oil") {
            prop->OilRate.update(value);
            prop->addProductionControl( Well::ProducerCMode::ORAT );
        }
        else if (variable == "gas") {
            prop->GasRate.update(value);
            prop->addProductionControl( Well::ProducerCMode::GRAT );
        }
        else {
            throw py::value_error("Unknown variable: " + variable);
        }
        UDQActive &udq_active = const_cast<UDQActive&>(sch.getUDQActive(index));
        if (prop->updateUDQActive(sch.getUDQConfig(index), udq_active)) {
            sch.updateUDQActive(udq_active, index);
        }
        well.updateProduction(prop);
        //well.setInsertIndex(index);
        sch.addWell(std::move(well), index);
        WellGroupEvents &events = sch.getWellGroupEvents(index);
        events.addEvent( well_name, ScheduleEvents::PRODUCTION_UPDATE);
        sch.addEvent(ScheduleEvents::PRODUCTION_UPDATE, index);
    }
}


void python::common::export_Schedule(py::module& module) {


    py::class_<ScheduleState>(module, "ScheduleState")
        .def_property_readonly("nupcol", py::overload_cast<>(&ScheduleState::nupcol, py::const_))
        .def("group", &get_group, ref_internal);


    // Note: In the below class we std::shared_ptr as the holder type, see:
    //
    //  https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
    //
    // this makes it possible to share the returned object with e.g. and
    //   opm.simulators.BlackOilSimulator Python object
    //
    py::class_< Schedule, std::shared_ptr<Schedule> >( module, "Schedule")
    .def(py::init<const Deck&, const EclipseState& >())
    .def("_groups", &get_groups )
    .def_property_readonly( "start",  &get_start_time )
    .def_property_readonly( "end",    &get_end_time )
    .def_property_readonly( "timesteps", &get_timesteps )
    .def("__len__", &Schedule::size)
    .def("__getitem__", &getitem)
    .def( "shut_well", &Schedule::shut_well)
    .def( "open_well", &Schedule::open_well)
    .def( "stop_well", &Schedule::stop_well)
    .def( "get_wells", &Schedule::getWells)
    .def("well_names", py::overload_cast<const std::string&>(&Schedule::wellNames, py::const_))
    .def( "get_well", &get_well)
    .def( "get_well_production_target", &get_well_production_target,
        py::arg("well_name"), py::arg("step"), py::arg("variable"))
    .def( "set_well_production_target", &set_well_production_target,
        py::arg("well_name"), py::arg("step"), py::arg("variable"), py::arg("value"))
    .def( "__contains__", &has_well );

}
