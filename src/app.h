#pragma once

#include <algorithm>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <chrono> 

#include "model.h"
#include "postgres.h"
#include "tagged.h"



namespace serialization {

class PlayerTokensRepr;
class PlayersRepr;
class ApplicationRepr;

}

namespace app {

namespace detail {
struct TokenTag {};
}

// Токен — сущность уровня приложения (аутентификация)
using Token = util::Tagged<std::string, detail::TokenTag>;
enum class ApiError {InvalidName, MapNotFound, TokenUnknown};

std::string HexEncode(uint64_t val);


class Player {
public:
    using Id = model::Dog::Id;
    using DogPtr = std::shared_ptr<model::Dog>;
    using SessionPtr = std::shared_ptr<model::GameSession>;

    Player(DogPtr dog, SessionPtr session);

    const Id& GetId() const noexcept;
    const std::string GetName() const noexcept;
    DogPtr GetDog() const noexcept;
    SessionPtr GetSession() const noexcept;

private:

    DogPtr dog_;
    SessionPtr session_;
};


class PlayerTokens {
public:
    friend class serialization::PlayerTokensRepr;

    using PlayerPtr = std::shared_ptr<Player>;
    using TokenToPlayer = std::unordered_map<Token, PlayerPtr, util::TaggedHasher<Token>>;

    Token AddPlayer(PlayerPtr player);
    PlayerPtr FindPlayer(const Token& token) const;
    Token GenerateToken();

    std::optional<Token> FindTokenByPlayer(PlayerPtr player_ptr) const;

    void DeletePlayerTokens(PlayerPtr player);

private:

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    TokenToPlayer tokens_;
};


class Players {
public:
    friend class serialization::PlayersRepr;

    using Id = Player::Id;
    using PlayerPtr = std::shared_ptr<Player>;
    using SessionPtr = std::shared_ptr<model::GameSession>;
    using PlayerMap = std::unordered_map<Id, PlayerPtr, util::TaggedHasher<Id>>;

    std::pair<PlayerPtr, Token> AddPlayer(PlayerPtr player);
    PlayerPtr FindPlayer(Id id) const;
    PlayerPtr FindPlayerByToken(const Token& token);

    void DeletePlayer(Id id);

private:

    PlayerMap players_;
    PlayerTokens player_tokens_;
    uint32_t next_player_ = 0;
};


class ListMapsUseCase {
 public:
    explicit ListMapsUseCase(model::Game& game);
    const model::Game::Maps& GetList() const;
private:
    model::Game& game_;   
};


class GetMapUseCase {
 public:
    explicit GetMapUseCase(model::Game& game);
    const model::Map* Find(std::string_view map_id_str) const;
private:
    model::Game& game_;    
};


class JoinGameUseCase {
 public:
    explicit JoinGameUseCase(model::Game& game, Players& players, bool random_pos_generate);

    struct JoinGameResult{
        Token token_;
        Player::Id user_id;
    };

    JoinGameResult Join(std::string map_id_str, std::string name_str);

private:
    model::Game& game_;
    Players& players_;
    bool random_pos_generate_;
};


class ListPlayersUseCase {
 public:
    explicit ListPlayersUseCase(model::Game& game, Players& players);
    const model::GameSession::Dogs& List(const Token& token) const;
private:
    model::Game& game_;
    Players& players_;
};


class GameStateUseCase {
public:
    explicit GameStateUseCase(Players& players);
    const model::GameSession::GameStateData GetState(const Token& token) const;
private:
    Players& players_;
};


class PlayerStateActionUseCase {
public:
    explicit PlayerStateActionUseCase(Players& players);
    void SetAction(const Token& token, std::string_view move_direction);
private:
    Players& players_;
};


class GameTickUseCase {
public:
    explicit GameTickUseCase(model::Game& game, Players& players, postgres_database::DataBase& game_db);
    void UpdateState(std::chrono::milliseconds time);
private:
    model::Game& game_;
    Players& players_;
    postgres_database::DataBase& game_db_;
};


class ApplicationListener {
public:
    virtual ~ApplicationListener() = default;
    virtual void OnTick(std::chrono::milliseconds delta) = 0;
};


class RecordsUseCase {
public:
    explicit RecordsUseCase(const postgres_database::DataBase& game_db);
    std::vector<domain::RetiredPlayer> GetRecords(int offset, int max_elements) const;
private:
    const postgres_database::DataBase& game_db_;
};


class Application {
public:
    friend class serialization::ApplicationRepr;

    explicit Application(model::Game& game, Players& players, postgres_database::DataBaseConfig db_config);

    Players& GetPlayers();
    const model::Game::Maps& ListMaps() const;
    const model::Map* FindMap(std::string_view map_id_str) const;
    const JoinGameUseCase::JoinGameResult JoinGame(std::string_view map_id_str, std::string_view name_str);

    const model::GameSession::Dogs& ListPlayers(const Token& token) const;
    const model::GameSession::GameStateData GameState(const Token& token) const;

    void SetPlayerAction(const Token& token, std::string_view move_direction);

    bool IsAutoTickEnabled() const;
    void SetAutoTickEnabled(bool enabled);
    void Tick(std::chrono::milliseconds delta);
    void SetGenerateRandPos(bool enabled);

    void SetApplicationListener(std::unique_ptr<ApplicationListener> listener);

    const std::vector<domain::RetiredPlayer> Records(int offset, int max_elements) const;

private:
    model::Game& game_;
    Players& players_;

    ListMapsUseCase list_maps_;
    GetMapUseCase get_map_;
    JoinGameUseCase join_game_;

    ListPlayersUseCase list_players_;
    GameStateUseCase game_state_;
    PlayerStateActionUseCase player_state_action_;
    GameTickUseCase game_tick_;

    bool auto_tick_enabled_ = false;
    bool randomize_spavn_dogs_ = false;

    std::unique_ptr<ApplicationListener> listener_ = nullptr;

    postgres_database::DataBase game_db_;
    RecordsUseCase records_;
};

}