#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json_loader {

void LoadRoads(const boost::json::array& roads_data, model::Map& map);
void LoadBuildings(const boost::json::array& buildings_data, model::Map& map);
void LoadOffices(const boost::json::array& offices_data, model::Map& map);

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
