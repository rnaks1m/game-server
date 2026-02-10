#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

#include "collision_detector.h"
#include "extra_data.h"
#include "loot_generator.h"
#include "tagged.h"



namespace json_fields {

namespace road_fields {
    constexpr const char* X0 = "x0";
    constexpr const char* Y0 = "y0";
    constexpr const char* X1 = "x1";
    constexpr const char* Y1 = "y1";
}

namespace building_fields {
    constexpr const char* X = "x";
    constexpr const char* Y = "y";
    constexpr const char* WIDTH = "w";
    constexpr const char* HEIGHT = "h";
}

namespace office_fields {
    constexpr const char* ID = "id";
    constexpr const char* X = "x";
    constexpr const char* Y = "y";
    constexpr const char* OFFSET_X = "offsetX";
    constexpr const char* OFFSET_Y = "offsetY";
}

namespace map_fields {
    constexpr const char* ID = "id";
    constexpr const char* NAME = "name";
    constexpr const char* ROADS = "roads";
    constexpr const char* BUILDINGS = "buildings";
    constexpr const char* OFFICES = "offices";
    constexpr const char* SPEED = "dogSpeed";
    constexpr const char* LOOT_TYPES = "lootTypes";
    constexpr const char* BAG_CAPACITY = "bagCapacity";
}

namespace root_fields {
    constexpr const char* MAPS = "maps";
    constexpr const char* DEFAULT_SPEED = "defaultDogSpeed";
    constexpr const char* LOOT_GENERATOR = "lootGeneratorConfig";
    constexpr const char* DEFAULT_BAG_CAPACITY = "defaultBagCapacity";
    constexpr const char* DOG_RETIREMENT_TIME = "dogRetirementTime";
}

namespace loot_gen_fields {
    constexpr const char* PERIOD = "period";
    constexpr const char* PROBABILITY = "probability";
}

} // namespace json_fields

namespace move_direction {
    constexpr const char* LEFT = "L";
    constexpr const char* RIGHT = "R";
    constexpr const char* UP = "U";
    constexpr const char* DOWN = "D";
    constexpr const char* STOP = "";
}

namespace serialization {

class DogRepr;
class LootRepr;
class GameSessionRepr;

}

namespace model {


constexpr double DOG_WIDTH = 0.6;
constexpr double LOOT_WIDTH = 0.0;
constexpr double OFFICE_WIDTH = 0.5;


using Dimension = int;
using Coord = Dimension;

using DimensionDouble = double;
using DCoord = DimensionDouble;

using namespace std::chrono_literals;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};


struct Position {
    DCoord x, y;
};

struct Speed {
    DCoord x, y;
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    NONE
};

const double ROAD_WIDTH_HALF = 0.4;
const double EPSILON = 1e-6;

std::string DirectionToString(Direction dir);

struct lootGeneratorConfig {
    double period;
    double probability;
};

struct RetiredPlayersInfo {
    std::string name;
    int score;
    int play_time;
};


class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

    struct RoadCoord {
        double min_x;
        double max_x;        
        double min_y;
        double max_y;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;
    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

    bool IsPointOnRoad(const Position& pos) const noexcept;
    RoadCoord GetRoadCoord() const noexcept;

private:
    Point start_;
    Point end_;
};


class Building {
public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept;
private:
    Rectangle bounds_;
};


class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept;
    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};


class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    struct RoadIndex {
        size_t road_idx;
        double coord;
    };

    Map(Id id, std::string name, const extra_data::ExtraData& parse_extra_data) noexcept;

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    double GetDogSpeed() const noexcept;
    size_t GetBagCapacity() const noexcept;
    size_t GetPointsByType(size_t type) const noexcept;

    const std::vector<RoadIndex>& GetHorizontalRoadsByY() const noexcept;
    const std::vector<RoadIndex>& GetVerticalRoadsByX() const noexcept;

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    void SetDogSpeed(double speed);
    void SetBagCapacity(size_t bag_capacity);

    void BuildRoadIndexes();

    const extra_data::ExtraData& GetExtraData() const noexcept;
    size_t GetCountTypes() const noexcept;
    
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::vector<RoadIndex> horizontal_roads_by_y_;
    std::vector<RoadIndex> vertical_roads_by_x_;

    double dog_speed_ = 0;
    size_t bag_capacity_ = 3;

    extra_data::ExtraData extra_data_;
};


class Loot {
public:
    friend class serialization::LootRepr;

    using Id = util::Tagged<uint64_t, Loot>;

    explicit Loot(Position position, Id id, size_t type);
    Id GetId() const noexcept;
    Position GetPosition() const noexcept;
    size_t GetType() const noexcept;

private:
    Position position_;
    Id id_;
    size_t type_;
};


struct LootItem {
    Loot::Id id{0u};
    size_t type = 0;
};

class Bag {
public:
    explicit Bag(size_t capacity);
    
    bool IsFull() const noexcept;
    size_t GetSize() const noexcept;
    size_t GetCapacity() const noexcept;
    
    bool AddItem(Loot::Id id, size_t type);
    void Clear();
    
    const std::vector<LootItem>& GetItems() const noexcept;

private:
    std::vector<LootItem> items_;
    size_t capacity_;
};


class Dog {
public:
    friend class serialization::DogRepr;

    using Id = util::Tagged<std::uint64_t, Dog>;

    Dog(Id id, std::string name, Position position, size_t bag_capacity);

    void SetDefaultSpeed(double speed);
    void SetSpeed(Speed speed);
    void SetPosition(Position position);
    void SetDirection(Direction direction);
    void IncreaseScore(size_t points);

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Position GetPosition() const noexcept;
    const Speed GetSpeed() const noexcept;
    const Direction GetDirection() const noexcept;
    const size_t GetScore() const noexcept;
    size_t GetBagCapacity() const noexcept;

    Position Move(double delta_time, const Map& roads);
    void Stop() noexcept;

    bool AddItemToBag(Loot::Id id, size_t type);
    void ClearBag();
    const std::vector<LootItem>& GetItemsFromBag() const;

    bool IsLeave(std::chrono::milliseconds time, std::chrono::milliseconds max_retired);
    int GetLeaveTime() const;

private:

    void MoveOnHorRoad(Position& clamped_pos, const Position& next_pos, const Road& road);
    void MoveOnVertRoad(Position& clamped_pos, const Position& next_pos, const Road& road);

    Id id_;
    std::string name_;
    Position position_;
    double default_speed_ = 1.0;
    Speed speed_ = {0.0, 0.0};
    Direction direction_ = Direction::NORTH;
    size_t bag_capacity_ = 3;
    Bag bag_;
    size_t score_ = 0;

    std::chrono::milliseconds in_game_ = 0ms;
    std::chrono::milliseconds retired_ = 0ms;
};



class GameSession;

class ItemGathererProviderImpl : public collision_detector::ItemGathererProvider {
public:

    using DogPtr = std::shared_ptr<Dog>;
    using LootPtr = std::shared_ptr<Loot>;
    using Loots = std::unordered_map<Loot::Id, LootPtr, util::TaggedHasher<Loot::Id>>;

    struct Movement {
        Position start;
        Position stop;
        DogPtr dog;
    };

    enum class Type { 
        LOOT, 
        OFFICE 
    };

    struct ObjectInfo {
        Type type;
        geom::Point2D position;
        double width;
        Loot::Id loot_id;
        Office::Id office_id;
    };

    ItemGathererProviderImpl(const std::vector<Movement>& movements, const Loots& loots, const Map::Offices& offices);

    size_t ItemsCount() const override;
    collision_detector::Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    collision_detector::Gatherer GetGatherer(size_t idx) const override;

    const ObjectInfo& GetObjectInfo(size_t idx) const;

private:
    std::vector<Movement> movements_;
    const Loots& loots_;
    const Map::Offices& offices_;

    std::vector<ObjectInfo> objects_;
};


class ItemCollector {
public:
    enum class EventType { 
        COLLECT, 
        RETURN 
    };

    struct CollectionEvent {      
        EventType type;
        Dog::Id dog_id;
        Loot::Id loot_id;
        Office::Id office_id;
        double time;
        
        // Для сортировки
        bool operator<(const CollectionEvent& other) const {
            return time < other.time;
        }
    };

    ItemCollector(const Map& map);

    std::vector<Loot::Id> CollectItems(GameSession& session ,std::vector<ItemGathererProviderImpl::Movement> movements);

private:

    void ProcessGatherEvents(GameSession& session, const std::vector<ItemGathererProviderImpl::Movement>& movements, const ItemGathererProviderImpl& provider);   
    void ProcessSequentialEvents(GameSession& session); 

    const Map& map_;
    std::vector<CollectionEvent> all_events_;
    std::vector<Loot::Id> collect_items_;
};


class GameSession {    
public:
    friend class serialization::GameSessionRepr;

    using Id = util::Tagged<std::string, GameSession>;
    using DogPtr = std::shared_ptr<Dog>;
    using Dogs = std::unordered_map<Dog::Id, DogPtr, util::TaggedHasher<Dog::Id>>;
    using LootPtr = std::shared_ptr<Loot>;
    using Loots = std::unordered_map<Loot::Id, LootPtr, util::TaggedHasher<Loot::Id>>;

    struct GameStateData {
        Loots loots;
        Dogs dogs;
    };

    explicit GameSession(Id id, const Map& map, lootGeneratorConfig config, double retirement_time);

    const Map& GetMap() const noexcept;
    const Dogs& GetDogs() const noexcept;

    DogPtr AddDog(std::string name, bool random_spavn);
    std::vector<DogPtr> UpdateState(std::chrono::milliseconds time);

    void AddLoot(LootPtr loot); 
    Loots GetLoot() const noexcept;

    GameStateData GetGameState() const noexcept;

    void GenerateLoot(std::chrono::milliseconds time_interval);

    void DeletePlayer(DogPtr dog);

private:

    Position GenerateRandomPosition();
    int GenerateRandomNumber(int num);
    
    Id id_;
    const Map& map_;

    Dogs dogs_;
    uint64_t next_dog_id_ = 0;

    Loots loots_;
    uint64_t next_loot_id_ = 0;
    loot_gen::LootGenerator loot_generator_;
    ItemCollector item_collector_;
    std::chrono::milliseconds retirement_time_;
};


class Game {
public:
    using Maps = std::vector<Map>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using SessionPtr = std::shared_ptr<GameSession>;
    using Sessions = std::unordered_map<Map::Id, SessionPtr, MapIdHasher>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    
    void AddMap(Map map);
    const Map* FindMap(const Map::Id& id) const noexcept;
    const Maps& GetMaps() const noexcept;

    const Sessions& GetSessions() const noexcept;
    SessionPtr FindOrAddGameSession(const Map::Id& map_id);

    void SetSpeed(double speed);
    double GetSpeed() const noexcept;

    void SetLootGenConfig(lootGeneratorConfig config);

    void SetDefBagCapacity(size_t def_bag_capacity);
    size_t GetDefBagCapacity() const noexcept;

    void SetRetirementTime(double time);
    double GetRetirementTime() const;

private:

    Maps maps_;
    Sessions sessions_;
    MapIdToIndex map_id_to_index_;
    double default_speed_ = 1.0;
    lootGeneratorConfig loot_gen_config_;
    size_t def_bag_capacity_ = 3;
    double retirement_time_ = 60.0;
};

}  // namespace model
