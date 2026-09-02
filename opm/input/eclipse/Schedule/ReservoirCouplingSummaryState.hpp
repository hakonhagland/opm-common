/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifndef OPM_RESERVOIR_COUPLING_SUMMARY_STATE_HPP
#define OPM_RESERVOIR_COUPLING_SUMMARY_STATE_HPP

#include <map>
#include <string>

namespace Opm {

/// Summary values imported from a reservoir-coupling master.
///
/// Values use the same output/deck unit convention as SummaryState. This
/// object contains no local simulator state and is not part of SummaryState
/// serialization.
class ReservoirCouplingSummaryState
{
public:
    using Values = std::map<std::string, double>;
    using GroupValues = std::map<std::string, Values>;

    void clear()
    {
        this->values_.clear();
        this->group_values_.clear();
    }

    void set(const std::string& keyword, const double value)
    {
        this->values_[keyword] = value;
    }

    void setGroupValue(const std::string& group,
                       const std::string& keyword,
                       const double value)
    {
        this->group_values_[group][keyword] = value;
    }

    bool has(const std::string& keyword) const
    {
        return this->values_.count(keyword) > 0;
    }

    bool hasGroupValue(const std::string& group,
                       const std::string& keyword) const
    {
        const auto group_pos = this->group_values_.find(group);
        return group_pos != this->group_values_.end()
            && group_pos->second.count(keyword) > 0;
    }

    double get(const std::string& keyword) const
    {
        return this->values_.at(keyword);
    }

    double getGroupValue(const std::string& group,
                         const std::string& keyword) const
    {
        return this->group_values_.at(group).at(keyword);
    }

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->values_);
        serializer(this->group_values_);
    }

private:
    Values values_;
    GroupValues group_values_;
};

} // namespace Opm

#endif // OPM_RESERVOIR_COUPLING_SUMMARY_STATE_HPP
