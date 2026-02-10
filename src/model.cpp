#include "model.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace model {

using namespace std::literals;


std::string DirectionToString(Direction dir) {
    switch (dir) {
        case Direction::NORTH: return "U";
        case Direction::SOUTH: return "D";
        case Direction::WEST: return "L";
        case Direction::EAST: return "R";
        default: return "U";
    }
}


    Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road::Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool Road::IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool Road::IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point Road::GetStart() const noexcept {
        return start_;
    }

    Point Road::GetEnd() const noexcept {
        return end_;
    }

    bool Road::IsPointOnRoad(const Position& pos) const noexcept {
        RoadCoord coord = GetRoadCoord();

        if (IsHorizontal()) {        
            if (pos.y < coord.min_y - EPSILON || pos.y > coord.max_y + EPSILON) {
                return false;
            }
            
            if (pos.x < coord.min_x - EPSILON || pos.x > coord.max_x + EPSILON) {
                return false;
            }
        } 
        else { 
            if (pos.x < coord.min_x - EPSILON || pos.x > coord.max_x + EPSILON) {
                return false;
            }

            if (pos.y < coord.min_y - EPSILON || pos.y > coord.max_y + EPSILON) {
                return false;
            }
        }
        return true;
    }

    Road::RoadCoord Road::GetRoadCoord() const noexcept {
        RoadCoord coord;

        if (IsHorizontal()) {
            coord.min_x = static_cast<double>(std::min(start_.x, end_.x)) - ROAD_WIDTH_HALF;
            coord.max_x = static_cast<double>(std::max(start_.x, end_.x)) + ROAD_WIDTH_HALF;
            coord.min_y = static_cast<double>(start_.y) - ROAD_WIDTH_HALF;
            coord.max_y = static_cast<double>(start_.y) + ROAD_WIDTH_HALF;
        }
        else {
            coord.min_x = static_cast<double>(start_.x) - ROAD_WIDTH_HALF;
            coord.max_x = static_cast<double>(start_.x) + ROAD_WIDTH_HALF;
            coord.min_y = static_cast<double>(std::min(start_.y, end_.y)) - ROAD_WIDTH_HALF;
            coord.max_y = static_cast<double>(std::max(start_.y, end_.y)) + ROAD_WIDTH_HALF;
        }

        return coord;
    }


    Building::Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& Building::GetBounds() const noexcept {
        return bounds_;
    }


    Office::Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Office::Id& Office::GetId() const noexcept {
        return id_;
    }

    Point Office::GetPosition() const noexcept {
        return position_;
    }

    Offset Office::GetOffset() const noexcept {
        return offset_;
    }


    Map::Map(Id id, std::string name, const extra_data::ExtraData& parse_extra_data) noexcept
        : id_(std::move(id))
        , name_(std::move(name)),
        extra_data_(parse_extra_data) {}

    const Map::Id& Map::GetId() const noexcept {
        return id_;
    }

    const std::string& Map::GetName() const noexcept {
        return name_;
    }

    const Map::Buildings& Map::GetBuildings() const noexcept {
        return buildings_;
    }

    const Map::Roads& Map::GetRoads() const noexcept {
        return roads_;
    }

    const Map::Offices& Map::GetOffices() const noexcept {
        return offices_;
    }

    void Map::AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void Map::AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } 
        catch (const std::exception& e) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    double Map::GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    void Map::SetDogSpeed(double speed) {
        dog_speed_ = speed;
    }

    void Map::BuildRoadIndexes() {
        for (size_t i = 0; i < roads_.size(); ++i) {
            const Road& road = roads_[i];

            if (road.IsHorizontal()) {
                horizontal_roads_by_y_.push_back({i, static_cast<double>(road.GetStart().y)});
            } 
            else { 
                vertical_roads_by_x_.push_back({i, static_cast<double>(road.GetStart().x)});
            }
        }
        
        std::sort(horizontal_roads_by_y_.begin(), horizontal_roads_by_y_.end(), 
            [](const RoadIndex& a, const RoadIndex& b) { return a.coord < b.coord; });
        
        std::sort(vertical_roads_by_x_.begin(), vertical_roads_by_x_.end(), 
            [](const RoadIndex& a, const RoadIndex& b) { return a.coord < b.coord; });
    }

    const std::vector<Map::RoadIndex>& Map::GetHorizontalRoadsByY() const noexcept { 
        return horizontal_roads_by_y_; 
    }
    
    const std::vector<Map::RoadIndex>& Map::GetVerticalRoadsByX() const noexcept { 
        return vertical_roads_by_x_; 
    }

    const extra_data::ExtraData& Map::GetExtraData() const noexcept {
        return extra_data_;
    }

    size_t Map::GetCountTypes() const noexcept {
        return extra_data_.GetSize();
    }

    size_t Map::GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    void Map::SetBagCapacity(size_t bag_capacity) {
        bag_capacity_ = bag_capacity;
    }

    size_t Map::GetPointsByType(size_t type) const noexcept {
        return extra_data_.GetValue(type);
    }


    Bag::Bag(size_t capacity) : capacity_(capacity) {}
    
    bool Bag::IsFull() const noexcept { 
        return items_.size() >= capacity_; 
    }

    size_t Bag::GetSize() const noexcept { 
        return items_.size(); 
    }

    size_t Bag::GetCapacity() const noexcept { 
        return capacity_; 
    }
    
    bool Bag::AddItem(Loot::Id id, size_t type) {
        if (IsFull()) return false;
        LootItem item{id, type};
        items_.emplace_back(item);
        return true;
    }
    
    void Bag::Clear() { 
        items_.clear(); 
    }
    
    const std::vector<LootItem>& Bag::GetItems() const noexcept { 
        return items_; 
    }


    Dog::Dog(Id id, std::string name, Position position, size_t bag_capacity) 
        : id_(id), name_(std::move(name)), position_(position), bag_capacity_(bag_capacity), bag_(bag_capacity) {}

    const Dog::Id& Dog::GetId() const noexcept { 
        return id_; 
    }

    const std::string& Dog::GetName() const noexcept { 
        return name_; 
    }

    const Position Dog::GetPosition() const noexcept { 
        return position_; 
    }

    const Speed Dog::GetSpeed() const noexcept {
        return speed_;
    }

    const Direction Dog::GetDirection() const noexcept {
        return direction_;
    }

    void Dog::SetDefaultSpeed(double speed) {
        default_speed_ = speed;
    }

    void Dog::SetSpeed(Speed speed) {
        speed_ = speed;
    }

    void Dog::SetPosition(Position position) {
        position_ = position;
    }

    void Dog::SetDirection(Direction direction) {
        direction_ = direction;
    }

    void Dog::Stop() noexcept {
        speed_ = {0.0, 0.0};
    }

    void Dog::MoveOnHorRoad(Position& clamped_pos, const Position& next_pos, const Road& road) {
        auto coord = road.GetRoadCoord();

        if (clamped_pos.y != next_pos.y) {
            if (next_pos.y > coord.max_y + EPSILON) {
                clamped_pos.y = coord.max_y;
            }
            else if (next_pos.y < coord.min_y - EPSILON) {
                clamped_pos.y = coord.min_y;
            }
            else {
                clamped_pos.y = next_pos.y;
            }
        }

        if (clamped_pos.x != next_pos.x) {
            if (next_pos.x > coord.max_x + EPSILON) {
                clamped_pos.x = coord.max_x;
            }
            else if (next_pos.x < coord.min_x - EPSILON) {
                clamped_pos.x = coord.min_x;
            }
            else {
                clamped_pos.x = next_pos.x;
            }
        }
    }

    void Dog::MoveOnVertRoad(Position& clamped_pos, const Position& next_pos, const Road& road) {
        auto coord = road.GetRoadCoord();

        if (clamped_pos.x != next_pos.x) {
            if (next_pos.x > coord.max_x + EPSILON) {
                clamped_pos.x = coord.max_x;
            }
            else if (next_pos.x < coord.min_x - EPSILON) {
                clamped_pos.x = coord.min_x;
            }
            else {
                clamped_pos.x = next_pos.x;
            }
        }
        
        if (clamped_pos.y != next_pos.y) {
            if (next_pos.y > coord.max_y + EPSILON) {
                clamped_pos.y = coord.max_y;
            }
            else if (next_pos.y < coord.min_y - EPSILON) {
                clamped_pos.y = coord.min_y;
            }
            else {
                clamped_pos.y = next_pos.y;
            }
        }
    }

    Position Dog::Move(double delta_time, const Map& map) {
        Position next_pos;
        Position clamped_pos = position_;

        next_pos.x = position_.x + speed_.x * delta_time;
        next_pos.y = position_.y + speed_.y * delta_time;

        const auto& roads = map.GetRoads();

        for (const auto& road : map.GetHorizontalRoadsByY()) {
            if (clamped_pos.x == next_pos.x && clamped_pos.y == next_pos.y) {
                break;
            }

            auto road_to_move = roads.at(road.road_idx);

            if (road_to_move.IsPointOnRoad(clamped_pos)) {
                if (road_to_move.IsHorizontal()) {
                    MoveOnHorRoad(clamped_pos, next_pos, road_to_move);
                }
                else {
                    MoveOnVertRoad(clamped_pos, next_pos, road_to_move);
                }
            }
        }

        for (const auto& road : map.GetVerticalRoadsByX()) {
            if (clamped_pos.x == next_pos.x && clamped_pos.y == next_pos.y) {
                break;
            }

            auto road_to_move = roads.at(road.road_idx);

            if (road_to_move.IsPointOnRoad(clamped_pos)) {
                if (road_to_move.IsHorizontal()) {
                    MoveOnHorRoad(clamped_pos, next_pos, road_to_move);
                }
                else {
                    MoveOnVertRoad(clamped_pos, next_pos, road_to_move);
                }
            }
        }

        if (clamped_pos.x != next_pos.x || clamped_pos.y != next_pos.y) {
            Stop();
        }

        position_.x = clamped_pos.x;
        position_.y = clamped_pos.y;

        return clamped_pos;
    }

    bool Dog::AddItemToBag(Loot::Id id, size_t type) {
        return bag_.AddItem(id, type);
    }

    void Dog::ClearBag() {
        bag_.Clear();
    }

    const std::vector<LootItem>& Dog::GetItemsFromBag() const {
        return bag_.GetItems();
    }

    void Dog::IncreaseScore(size_t points) {
        score_ += points;
    }

    const size_t Dog::GetScore() const noexcept {
        return score_;
    }

    size_t Dog::GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    bool Dog::IsLeave(std::chrono::milliseconds time, std::chrono::milliseconds max_retired) {
        in_game_ += time;

        if (speed_.x == 0.0 && speed_.y == 0.0) {
            retired_ += time;
        }
        else {
            retired_ = 0ms;
        }

        if (retired_ >= max_retired) {
            return false;
        }

        return true;
    }

    int Dog::GetLeaveTime() const {
        return in_game_.count();
    }


    Loot::Loot(Position position, Id id, size_t type) : position_(position), id_(id), type_(type) {}

    Loot::Id Loot::GetId() const noexcept {
        return id_;
    }

    Position Loot::GetPosition() const noexcept {
        return position_;
    }

    size_t Loot::GetType() const noexcept {
        return type_;
    }


    ItemGathererProviderImpl::ItemGathererProviderImpl(
        const std::vector<ItemGathererProviderImpl::Movement>& movements, 
        const ItemGathererProviderImpl::Loots& loots, 
        const Map::Offices& offices)
            : loots_(loots), offices_(offices), movements_(movements) {

            // Добавляем весь лут
            for (const auto& [id, loot] : loots_) {
                auto pos = loot->GetPosition();
                objects_.push_back({
                    Type::LOOT,
                    geom::Point2D{pos.x, pos.y},
                    LOOT_WIDTH,
                    id,
                    Office::Id{""}
                });
            }
            
            // Добавляем все офисы
            for (const auto& office : offices_) {
                auto pos = office.GetPosition();
                objects_.push_back({
                    Type::OFFICE,
                    geom::Point2D{static_cast<double>(pos.x), static_cast<double>(pos.y)},
                    OFFICE_WIDTH,
                    Loot::Id{0},
                    office.GetId()
                });
            }
    }

    size_t ItemGathererProviderImpl::ItemsCount() const {
        return objects_.size();
    }

    collision_detector::Item ItemGathererProviderImpl::GetItem(size_t idx) const {
        const auto& obj = objects_.at(idx);
        return collision_detector::Item{
            obj.position,
            obj.width
        };
    }

    size_t ItemGathererProviderImpl::GatherersCount() const {
        return movements_.size();
    }

    collision_detector::Gatherer ItemGathererProviderImpl::GetGatherer(size_t idx) const {
        const auto& movement = movements_[idx];

        return collision_detector::Gatherer{
            geom::Point2D{movement.start.x, movement.start.y},
            geom::Point2D{movement.stop.x, movement.stop.y},
            DOG_WIDTH
        };
    }

    const ItemGathererProviderImpl::ObjectInfo& ItemGathererProviderImpl::GetObjectInfo(size_t idx) const {
        return objects_.at(idx);
    }


    ItemCollector::ItemCollector(const Map& map) : map_(map) {}

    std::vector<Loot::Id> ItemCollector::CollectItems(GameSession& session ,std::vector<ItemGathererProviderImpl::Movement> movements) {
        all_events_.clear();
        collect_items_.clear();

        const auto& loots = session.GetLoot();
        const auto& offices = map_.GetOffices();
        
        if (movements.empty()) {
            return collect_items_;
        }

        ItemGathererProviderImpl provider(movements, loots, offices);

        // 1. Собираем ВСЕ события сбора и возврата
        ProcessGatherEvents(session, movements, provider);
        
        // 2. Обрабатываем события в хронологическом порядке
        ProcessSequentialEvents(session);

        return collect_items_;
    }

    void ItemCollector::ProcessGatherEvents(GameSession& session, const std::vector<ItemGathererProviderImpl::Movement>& movements, const ItemGathererProviderImpl& provider) {
        auto loots = session.GetLoot();

        auto gather_events = collision_detector::FindGatherEvents(provider);

        for (const auto& event : gather_events) {
            const auto& object_info = provider.GetObjectInfo(event.item_id);
            const auto& movement = movements.at(event.gatherer_id);
            auto& dog = movement.dog;

            if (object_info.type == ItemGathererProviderImpl::Type::LOOT) {
                all_events_.push_back({
                    EventType::COLLECT,
                    dog->GetId(),
                    object_info.loot_id,
                    Office::Id{""},
                    event.time
                });
            }
            else if (object_info.type == ItemGathererProviderImpl::Type::OFFICE) {
                all_events_.push_back({
                    EventType::RETURN,
                    dog->GetId(),
                    Loot::Id{0},
                    object_info.office_id,
                    event.time
                });
            }
        }
    }

    void ItemCollector::ProcessSequentialEvents(GameSession& session) {
        std::sort(all_events_.begin(), all_events_.end());

        auto loots = session.GetLoot();

        for (const auto& event : all_events_) {
            auto dog_it = session.GetDogs().find(event.dog_id);
            auto& dog = dog_it->second;

            if (event.type == EventType::COLLECT) {         
                // Проверяем, что предмет еще существует (не собран ранее)
                if (!loots.count(event.loot_id)) {
                    continue;
                }
                
                const auto& loot = loots.at(event.loot_id);

                if (dog->AddItemToBag(event.loot_id, loot->GetType())) {
                    loots.erase(event.loot_id);
                    collect_items_.push_back(event.loot_id);
                }
                
            } else if (event.type == EventType::RETURN) {
                if (!dog->GetItemsFromBag().empty()) {
                    for (const auto& item : dog->GetItemsFromBag()) {
                        dog->IncreaseScore(session.GetMap().GetPointsByType(item.type));
                    }

                    dog->ClearBag();
                }
            }

        }
    }


    GameSession::GameSession(Id id, const Map& map, lootGeneratorConfig config, double retirement_time) : 
        id_(std::move(id)), 
        map_(map), 
        loot_generator_(std::chrono::milliseconds(static_cast<int64_t>(config.period)), 
        config.probability), item_collector_(map),
        retirement_time_(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(retirement_time))) {}

    const Map& GameSession::GetMap() const noexcept {
        return map_;
    }

    const GameSession::Dogs& GameSession::GetDogs() const noexcept {
        return dogs_;;
    }

    GameSession::DogPtr GameSession::AddDog(std::string name, bool random_spavn) {
        Dog::Id dog_id{next_dog_id_++};
        Position pos = {0.0, 0.0};

        if (random_spavn) {
            pos = GenerateRandomPosition();
        }

        DogPtr dog = std::make_shared<Dog>(dog_id, std::move(name), pos, map_.GetBagCapacity());
        dogs_.emplace(dog_id, dog);
        return dog;
    }

    std::vector<GameSession::DogPtr> GameSession::UpdateState(std::chrono::milliseconds time) {
        std::vector<DogPtr> inactive_dogs;

        GenerateLoot(time);
        const double delta_time = static_cast<double>(time.count()) / 1000.0;
        std::vector<ItemGathererProviderImpl::Movement> dog_moves;
        
        for (const auto& [id, dog_ptr] : dogs_) {
            
            if (!dog_ptr->IsLeave(time, retirement_time_)) {
                inactive_dogs.push_back(dog_ptr);
            }

            Position start = dog_ptr->GetPosition();
            Position stop = dog_ptr->Move(delta_time, map_);

            size_t bag_capacity = map_.GetBagCapacity();
            dog_moves.emplace_back(ItemGathererProviderImpl::Movement{start, stop, dog_ptr});
        }

        auto collect_items = item_collector_.CollectItems(*this, dog_moves);

        for (const auto& item_id : collect_items) {
            loots_.erase(item_id);
        }

        return inactive_dogs;
    }

    GameSession::GameStateData GameSession::GetGameState() const noexcept {
        return {loots_, dogs_};
    }

    Position GameSession::GenerateRandomPosition() {
        const auto& roads = map_.GetRoads();

        if (roads.empty()) {
            return {0.0, 0.0};
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<size_t> road_index(0, roads.size() - 1);
        const Road& random_road = roads.at(road_index(gen));

        auto start_road = random_road.GetStart();
        auto end_road = random_road.GetEnd();

        Position start = {static_cast<double>(start_road.x), static_cast<double>(start_road.y)};
        Position end = {static_cast<double>(end_road.x), static_cast<double>(end_road.y)};

        auto coord = random_road.GetRoadCoord();
        Position random_pos;

        if (random_road.IsHorizontal()) {
            std::uniform_real_distribution<double> coord_x(coord.min_x, coord.max_x);
            std::uniform_real_distribution<double> coord_y(coord.min_y, coord.max_y);
            random_pos = {coord_x(gen), coord_y(gen)};
        }
        else {
            std::uniform_real_distribution<double> coord_x(coord.min_x, coord.max_x);
            std::uniform_real_distribution<double> coord_y(coord.min_y, coord.max_y);
            random_pos = {coord_x(gen), coord_y(gen)};
        }

        return random_pos;
    }

    int GameSession::GenerateRandomNumber(int num) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::uniform_int_distribution<int> dist(0, num);
        return dist(gen);
    }

    void GameSession::GenerateLoot(std::chrono::milliseconds time_interval) {
        auto count = loot_generator_.Generate(time_interval, loots_.size(), dogs_.size());

        for (auto i(0); i < count; ++i) {
            int type = GenerateRandomNumber(map_.GetCountTypes() - 1);
            Position pos = GenerateRandomPosition();
            Loot::Id id{next_loot_id_++};
            LootPtr loot = std::make_shared<Loot>(pos, id, static_cast<size_t>(type));
            loots_.emplace(id, loot);
        }
    }

    void GameSession::AddLoot(GameSession::LootPtr loot) {
        loots_.emplace(loot->GetId(), std::move(loot));
    }

    GameSession::Loots GameSession::GetLoot() const noexcept {
        return loots_;
    }

    void GameSession::DeletePlayer(DogPtr dog) {
        dogs_.erase(dog->GetId());
    }


    const Game::Maps& Game::GetMaps() const noexcept {
        return maps_;
    }

    const Map* Game::FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    void Game::AddMap(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } 
        else {
            try {
                maps_.emplace_back(std::move(map));
            } 
            catch (const std::exception& e) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::SetSpeed(double speed) {
        default_speed_ = speed;
    }

    double Game::GetSpeed() const noexcept {
        return default_speed_;
    }

    Game::SessionPtr Game::FindOrAddGameSession(const Map::Id& map_id) {
        auto it = sessions_.find(map_id);
        if (it != sessions_.end()) {
            return it->second;
        }

        if (const model::Map* map = FindMap(map_id)) {
            // ID сессии берется из ID карты
            GameSession::Id session_id{*map_id}; 
            SessionPtr session = std::make_shared<GameSession>(session_id, *map, loot_gen_config_, retirement_time_);
            sessions_.emplace(map_id, session);
            return session;
        }

        return nullptr;
    }

    const Game::Sessions& Game::GetSessions() const noexcept {
        return sessions_;
    }

    void Game::SetLootGenConfig(lootGeneratorConfig config) {
        loot_gen_config_ = config;
    }

    void Game::SetDefBagCapacity(size_t def_bag_capacity) {
        def_bag_capacity_ = def_bag_capacity;
    }

    size_t Game::GetDefBagCapacity() const noexcept {
        return def_bag_capacity_;
    }

    void Game::SetRetirementTime(double time) {
        retirement_time_ = time;
    }

    double Game::GetRetirementTime() const {
        return retirement_time_;
    }

}  // namespace model
