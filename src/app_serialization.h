#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>

#include "app.h"
#include "model_serialization.h"

namespace serialization {

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(app::Player& player) :
        dog_(*player.GetDog()), session_(*player.GetSession()) {}

    [[nodiscard]] app::Player Restore(const model::Game& game) const {
        auto dog = dog_.Restore();
        auto session_ptr = session_.Restore(game);
        return app::Player(std::make_shared<model::Dog>(dog), session_ptr);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& dog_;
        ar& session_;
    }
    
private:
    DogRepr dog_;
    GameSessionRepr session_;
};

class PlayerTokensRepr {
public:
    PlayerTokensRepr() = default;

    explicit PlayerTokensRepr(app::PlayerTokens& tokens_to_players) {
        for (const auto& [token, player] : tokens_to_players.tokens_) {
            PlayerRepr player_repr(*player);
            std::string token_str(*token);
            tokens_.emplace(token_str, player_repr);
        }
    }

    void Restore(app::PlayerTokens& tokens_to_players, const model::Game& game) const {
        tokens_to_players.tokens_.clear();

        for (const auto& [str, player_repr] : tokens_) {
            app::Token token(str);
            app::Player player = player_repr.Restore(game);
            auto player_ptr = std::make_shared<app::Player>(player);
            tokens_to_players.tokens_.emplace(token, player_ptr);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& tokens_;
    }

private:
    std::unordered_map<std::string, PlayerRepr> tokens_;
};

class PlayersRepr {
public:
    PlayersRepr() = default;

    explicit PlayersRepr(app::Players& players)
     : next_player_(players.next_player_), player_tokens_repr_(players.player_tokens_) {
        for (const auto& [id, player] : players.players_) {
            PlayerRepr player_repr(*player);
            uint64_t id_num = *id;
            players_.emplace(id_num, player_repr);
        }
    }

    void Restore(app::Players& players, const model::Game& game) const {
        players.next_player_ = next_player_;
        player_tokens_repr_.Restore(players.player_tokens_, game);
        players.players_.clear();

        for (const auto& [id_num, player_repr] : players_) {
            model::Dog::Id id{id_num};
            app::Player player = player_repr.Restore(game);
            auto player_ptr = std::make_shared<app::Player>(player);
            players.players_.emplace(id, player_ptr);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
        ar& player_tokens_repr_;
        ar& next_player_;
    }

private:
    std::unordered_map<uint64_t, PlayerRepr> players_;
    PlayerTokensRepr player_tokens_repr_;
    uint32_t next_player_ = 0;
};

class ApplicationRepr {
public:
    ApplicationRepr() = default;

    explicit ApplicationRepr(app::Application& application)
     : players_(application.players_)
     , auto_tick_enabled_(application.auto_tick_enabled_)
     , randomize_spavn_dogs_(application.randomize_spavn_dogs_) {}

     void Restore(app::Application& application) const {
        players_.Restore(application.players_, application.game_);
        application.auto_tick_enabled_ = auto_tick_enabled_;
        application.randomize_spavn_dogs_ = randomize_spavn_dogs_;
     }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
        ar& auto_tick_enabled_;
        ar& randomize_spavn_dogs_;
    }

private:
    PlayersRepr players_;
    bool auto_tick_enabled_ = false;
    bool randomize_spavn_dogs_ = false;
};


void AppSerialization(const std::filesystem::path& file_to_serialize_, app::Application& app);
void AppDeserialization(const std::filesystem::path& file_to_serialize_, app::Application& app);

}