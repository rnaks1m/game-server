#pragma once

#include <boost/serialization/vector.hpp>

#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Position& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

template <typename Archive>
void serialize(Archive& ar, LootItem& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
}

}  // namespace model

namespace serialization {

using namespace model;

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const Dog& dog)
        : id_(dog.id_)
        , name_(dog.name_)
        , pos_(dog.position_)
        , bag_capacity_(dog.bag_capacity_)
        , speed_(dog.speed_)
        , direction_(dog.direction_)
        , score_(dog.score_)
        , in_game_(std::chrono::duration_cast<std::chrono::duration<double>>(dog.in_game_).count())
        , retired_(std::chrono::duration_cast<std::chrono::duration<double>>(dog.retired_).count()) {
        
        for (const auto& item : dog.GetItemsFromBag()) {
            bag_.push_back(item);
        }
    }

    [[nodiscard]] Dog Restore() const {
        Dog dog{id_, name_, pos_, bag_capacity_};
        dog.speed_ = speed_;
        dog.direction_ = direction_;
        dog.score_ = score_;
        dog.in_game_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(in_game_));
        dog.retired_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(retired_));
        for (const auto& item : bag_) {
            if (!dog.AddItemToBag(item.id, item.type)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& bag_capacity_;
        ar& bag_;
        ar& score_;
        ar& in_game_;
        ar& retired_;
    }

private:
    Dog::Id id_ = Dog::Id{0u};
    std::string name_;
    Position pos_;
    double default_speed_ = 1.0;
    Speed speed_ = {0.0, 0.0};
    Direction direction_ = Direction::NORTH;
    size_t bag_capacity_ = 3;
    std::vector<LootItem> bag_;
    size_t score_ = 0;
    double in_game_;
    double retired_;
};
/* Другие классы модели сериализуются и десериализуются похожим образом */

// LootRepr - сериализованное представление класса Loot
class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const Loot& loot)
        : id_(loot.id_)
        , position_(loot.position_)
        , type_(loot.type_) {
    }

    [[nodiscard]] Loot Restore() const {
        return Loot{position_, id_, type_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& position_;
        ar& type_;
    }

private:
    Loot::Id id_ = Loot::Id{0u};
    Position position_;
    size_t type_ = 0;
};

// GameSessionRepr - сериализованное представление класса GameSession
class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const GameSession& session)
        : id_(session.id_)
        , map_id_(session.map_.GetId())
        , next_dog_id_(session.next_dog_id_)
        , next_loot_id_(session.next_loot_id_)
        , retirement_time_(std::chrono::duration_cast<std::chrono::duration<double>>(session.retirement_time_).count()) {
        
        for (const auto& [id, dog] : session.dogs_) {
            dogs_.emplace(*id, DogRepr{*dog});
        }
        
        for (const auto& [id, loot] : session.loots_) {
            loots_.emplace(*id, LootRepr{*loot});
        }
    }

    [[nodiscard]] std::shared_ptr<GameSession> Restore(const Game& game) const {
        const Map* map = game.FindMap(map_id_);
        if (!map) {
            throw std::runtime_error("Map not found for session restoration");
        }

        lootGeneratorConfig config;
        auto session = std::make_shared<GameSession>(id_, *map, config, retirement_time_);
        
        session->next_dog_id_ = next_dog_id_;
        session->next_loot_id_ = next_loot_id_;
        
        for (const auto& [id, dog_repr] : dogs_) {
            Dog dog = dog_repr.Restore();
            auto dog_ptr = std::make_shared<Dog>(std::move(dog));
            session->dogs_.emplace(dog_ptr->GetId(), dog_ptr);
        }
        
        for (const auto& [id, loot_repr] : loots_) {
            Loot loot = loot_repr.Restore();
            auto loot_ptr = std::make_shared<Loot>(std::move(loot));
            session->loots_.emplace(loot_ptr->GetId(), loot_ptr);
        }
        
        return session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& *map_id_;
        ar& next_dog_id_;
        ar& next_loot_id_;
        ar& dogs_;
        ar& loots_;
        ar& retirement_time_;
    }

private:
    GameSession::Id id_ = GameSession::Id{""};
    Map::Id map_id_ = Map::Id{""};
    uint64_t next_dog_id_ = 0;
    uint64_t next_loot_id_ = 0;
    std::unordered_map<uint64_t, DogRepr> dogs_; 
    std::unordered_map<uint64_t, LootRepr> loots_;
    double retirement_time_;
};

}  // namespace serialization
