#include <iostream>
#include <fstream> 
#include <string>

#include "json_loader.h"



namespace json_loader {

using namespace json_fields;

void LoadRoads(const boost::json::array& roads_data, model::Map& map) {

    for(const auto& road : roads_data) {    
        if(!road.is_object()) {
            continue;
        }

        auto& road_obj = road.as_object();

        if(!road_obj.contains(road_fields::X0) || !road_obj.contains(road_fields::Y0)) {
            continue;
        }

        int x0 = static_cast<int>(road_obj.at(road_fields::X0).get_int64());
        int y0 = static_cast<int>(road_obj.at(road_fields::Y0).get_int64());

        model::Point start{x0, y0};

        if(road_obj.contains(road_fields::X1)) {
            model::Coord end_x = static_cast<model::Coord>(road_obj.at(road_fields::X1).get_int64());
            map.AddRoad(model::Road(model::Road::HORIZONTAL, start, end_x));
        }
        else if (road_obj.contains(road_fields::Y1)) {
            model::Coord end_y = static_cast<model::Coord>(road_obj.at(road_fields::Y1).get_int64());
            map.AddRoad(model::Road(model::Road::VERTICAL, start, end_y));
        }
        else {
            continue;
        }
    }
}



void LoadBuildings(const boost::json::array& buildings_data, model::Map& map) {

    for(const auto& building : buildings_data) {
        if(!building.is_object()) {
            continue;
        }

        auto& building_obj = building.as_object();

        if(!building_obj.contains(building_fields::X) 
        || !building_obj.contains(building_fields::Y) 
        || !building_obj.contains(building_fields::WIDTH) 
        || !building_obj.contains(building_fields::HEIGHT)) {
            continue;
        }

        int x = static_cast<int>(building_obj.at(building_fields::X).get_int64());
        int y = static_cast<int>(building_obj.at(building_fields::Y).get_int64());
        int w = static_cast<int>(building_obj.at(building_fields::WIDTH).get_int64());
        int h = static_cast<int>(building_obj.at(building_fields::HEIGHT).get_int64());

        model::Rectangle rect{{x, y}, {w, h}};
        map.AddBuilding(model::Building(rect));
    }
}



void LoadOffices(const boost::json::array& offices_data, model::Map& map) {

    for(const auto& office : offices_data) {
        if(!office.is_object()) {
            continue;
        }

        auto& office_obj = office.as_object();

        if(!office_obj.contains(office_fields::ID) 
        || !office_obj.contains(office_fields::X) 
        || !office_obj.contains(office_fields::Y) 
        || !office_obj.contains(office_fields::OFFSET_X) 
        || !office_obj.contains(office_fields::OFFSET_Y)) {
            continue;
        }

        std::string id_str = std::string(office_obj.at(office_fields::ID).get_string());
        model::Office::Id id(std::move(id_str));

        int x = static_cast<int>(office_obj.at(office_fields::X).get_int64());
        int y = static_cast<int>(office_obj.at(office_fields::Y).get_int64());
        int offsetX = static_cast<int>(office_obj.at(office_fields::OFFSET_X).get_int64());
        int offsetY = static_cast<int>(office_obj.at(office_fields::OFFSET_Y).get_int64());

        model::Office ready_office({id, {x, y}, {offsetX, offsetY} });
        map.AddOffice(std::move(ready_office));
    }
}



model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::ifstream ifs(json_path);
    if(!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + json_path.string());
    }

    std::string file_data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    boost::json::value json_data;
    try {
        json_data = boost::json::parse(file_data);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Invalid JSON in file: " + json_path.string());
    }


    if (!json_data.as_object().contains(root_fields::LOOT_GENERATOR)) {
        throw std::runtime_error("Invalid JSON in file: " + json_path.string());
    }

    auto json_loot_config = json_data.at(root_fields::LOOT_GENERATOR).as_object();

    if (!json_loot_config.contains(loot_gen_fields::PERIOD)
     || !json_loot_config.contains(loot_gen_fields::PROBABILITY)) {
        throw std::runtime_error("Invalid JSON in file: " + json_path.string());
    }

    model::lootGeneratorConfig loot_config;
    loot_config.period = json_loot_config.at(loot_gen_fields::PERIOD).as_double();
    loot_config.probability = json_loot_config.at(loot_gen_fields::PROBABILITY).as_double();

    game.SetLootGenConfig(loot_config);

    if (json_data.as_object().contains(root_fields::DOG_RETIREMENT_TIME)) {
        double time = json_data.at(root_fields::DOG_RETIREMENT_TIME).as_double();
        game.SetRetirementTime(time);
    }


    if (json_data.as_object().contains(root_fields::DEFAULT_SPEED)) {
        double def_speed = json_data.at(root_fields::DEFAULT_SPEED).as_double();
        game.SetSpeed(def_speed);
    }

    if (json_data.as_object().contains(root_fields::DEFAULT_BAG_CAPACITY)) {
        auto def_speed = json_data.at(root_fields::DEFAULT_BAG_CAPACITY).get_int64();
        game.SetDefBagCapacity(static_cast<size_t>(def_speed));
    }

    auto json_maps = json_data.at(root_fields::MAPS).as_array();

    for(const auto& json_map : json_maps) {
        if(!json_map.is_object()) {
            continue;
        }
        //boost::json::object
        auto& map_obj = json_map.as_object();

        if(!map_obj.contains(map_fields::ID) 
        || !map_obj.contains(map_fields::NAME) 
        || !map_obj.contains(map_fields::ROADS)
        || !map_obj.contains(map_fields::LOOT_TYPES)) {
            continue;
        }

        auto types = map_obj.at(map_fields::LOOT_TYPES).as_array();
        if (types.empty()) {
            throw std::runtime_error("Invalid JSON in file: " + json_path.string());
        }

        std::string id_str = std::string(map_obj.at(map_fields::ID).get_string());
        model::Map::Id id(std::move(id_str));
        extra_data::ExtraData ex_data(types);

        std::string name_str = std::string(map_obj.at(map_fields::NAME).get_string());
        model::Map map(id, std::move(name_str), ex_data);

        if (map_obj.contains(map_fields::ROADS)) {
            auto roads_data = map_obj.at(map_fields::ROADS).as_array();

            if(roads_data.empty()) {
                continue;
            }

            LoadRoads(roads_data, map);
        }

        if (map_obj.contains(map_fields::BUILDINGS)) {
            auto buildings_data = map_obj.at(map_fields::BUILDINGS).as_array();
            LoadBuildings(buildings_data, map);
        }

        if (map_obj.contains(map_fields::OFFICES)) {
            auto offices_data = map_obj.at(map_fields::OFFICES).as_array();
            LoadOffices(offices_data, map);
        }

        if (map_obj.contains(map_fields::SPEED)) {
            double dog_speed = map_obj.at(map_fields::SPEED).as_double();
            map.SetDogSpeed(dog_speed);
        }
        else {
            double dog_speed = game.GetSpeed();
            map.SetDogSpeed(dog_speed);
        }

        if (map_obj.contains(map_fields::BAG_CAPACITY)) {
            auto bag_capacity = map_obj.at(map_fields::BAG_CAPACITY).get_int64();
            map.SetBagCapacity(static_cast<size_t>(bag_capacity));
        }
        else {
            size_t bag_capacity = game.GetDefBagCapacity();
            map.SetBagCapacity(bag_capacity);
        }

        map.BuildRoadIndexes();
        game.AddMap(std::move(map));
    }

    return game;
}

}  // namespace json_loader
