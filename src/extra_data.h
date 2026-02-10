#pragma once

#include <boost/json.hpp>

namespace extra_data {

class ExtraData {
public:

explicit ExtraData(boost::json::array loot_types);
boost::json::array GetLootTypes() const noexcept;
size_t GetSize() const noexcept;
size_t GetValue(size_t type) const noexcept;

private:
    boost::json::array loot_types_;
};

}