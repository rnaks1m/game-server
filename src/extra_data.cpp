#include "extra_data.h"

namespace extra_data {

ExtraData::ExtraData(boost::json::array loot_types) : loot_types_(loot_types) {}

boost::json::array ExtraData::GetLootTypes() const noexcept {
    return loot_types_;
}

size_t ExtraData::GetSize() const noexcept {
    return loot_types_.size();
}

size_t ExtraData::GetValue(size_t type) const noexcept {
    auto type_info = loot_types_.at(type).as_object();
    return static_cast<size_t>(type_info.at("value").get_int64());
}

}