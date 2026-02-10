#include "app.h"

namespace app {

    std::string HexEncode(uint64_t val) {
        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << val;
        return ss.str();
    }



    Player::Player(DogPtr dog, SessionPtr session) :
        dog_(std::move(dog)), session_(std::move(session)) {}

    const Player::Id& Player::GetId() const noexcept { 
        return dog_->GetId(); 
    }

    const std::string Player::GetName() const noexcept {
        return dog_->GetName();
    }

    Player::DogPtr Player::GetDog() const noexcept { 
        return dog_; 
    }

    Player::SessionPtr Player::GetSession() const noexcept { 
        return session_; 
    }

    Token PlayerTokens::GenerateToken() {
        uint64_t part1 = generator1_();
        uint64_t part2 = generator2_();
        std::string token_str = HexEncode(part1) + HexEncode(part2);
        return Token{std::move(token_str)};
    }    

    Token PlayerTokens::AddPlayer(PlayerPtr player) {
        Token token = GenerateToken();
        tokens_.emplace(token, player);
        return token;
    }

    PlayerTokens::PlayerPtr PlayerTokens::FindPlayer(const Token& token) const {
        auto it = tokens_.find(token);

        if (it != tokens_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void PlayerTokens::DeletePlayerTokens(PlayerPtr player) {
        auto token = FindTokenByPlayer(player);
        if (token) {
            tokens_.erase(*token);
        }
    }    

    std::optional<Token> PlayerTokens::FindTokenByPlayer(PlayerPtr player_ptr) const {
        for (const auto& [token, player] : tokens_) {
            if (player == player_ptr) {
                return token;
            }
        }
        return std::nullopt;
    }

    std::pair<Players::PlayerPtr, Token> Players::AddPlayer(PlayerPtr player) {
        Player::Id player_id = player->GetId();
        Token token = player_tokens_.AddPlayer(player);
        players_.emplace(player_id, player);
        return { player, token };
    }

    Players::PlayerPtr Players::FindPlayer(Id id) const {
        auto it = players_.find(id);

        if(it != players_.end()) {
            return it->second;
        }
        return nullptr;
    }

    Players::PlayerPtr Players::FindPlayerByToken(const Token& token) {
        return player_tokens_.FindPlayer(token);
    }

    void Players::DeletePlayer(Players::Id id) {
        if (auto it = players_.find(id); it != players_.end()) {
            player_tokens_.DeletePlayerTokens(it->second);
            players_.erase(it);
        }
    }


    ListMapsUseCase::ListMapsUseCase(model::Game& game) : game_(game) {}

    const model::Game::Maps& ListMapsUseCase::GetList() const {
        return game_.GetMaps();
    }


    GetMapUseCase::GetMapUseCase(model::Game& game) : game_(game) {}

    const model::Map* GetMapUseCase::Find(std::string_view map_id_str) const {
        model::Map::Id map_id{std::string(map_id_str)};
        return game_.FindMap(map_id);
    }


    JoinGameUseCase::JoinGameUseCase(model::Game& game, Players& players, bool random_pos_generate) 
        : game_(game), players_(players), random_pos_generate_(random_pos_generate) {}

    JoinGameUseCase::JoinGameResult JoinGameUseCase::Join(std::string map_id_str, std::string name_str) {
        model::Map::Id map_id{std::string(map_id_str)};
        auto session = game_.FindOrAddGameSession(map_id);

        if (session == nullptr) {
            throw ApiError::MapNotFound;
        }

        auto dog = session->AddDog(name_str, random_pos_generate_);
        app::Players::PlayerPtr player = std::make_shared<app::Player>(dog, session);
        auto [player_ptr, token ] = players_.AddPlayer(player);
        return { token, player_ptr->GetId() };
    }


    ListPlayersUseCase::ListPlayersUseCase(model::Game& game, Players& players) : game_(game), players_(players) {}

    const model::GameSession::Dogs& ListPlayersUseCase::List(const Token& token) const {
        std::shared_ptr<app::Player> player = players_.FindPlayerByToken(token);
        return player->GetSession()->GetDogs();
    }


    GameStateUseCase::GameStateUseCase(Players& players) : players_(players) {}

    const model::GameSession::GameStateData GameStateUseCase::GetState(const Token& token) const {
        Players::PlayerPtr player = players_.FindPlayerByToken(token);
        const auto& game_state = player->GetSession()->GetGameState(); 
        return game_state;
    }


    PlayerStateActionUseCase::PlayerStateActionUseCase(Players& players) : players_(players) {}

    void PlayerStateActionUseCase::SetAction(const Token& token, std::string_view move_direction) {
        auto player = players_.FindPlayerByToken(token);
        double default_speed = player->GetSession()->GetMap().GetDogSpeed();
        model::Speed speed = {0.0, 0.0};
        model::Direction dir;

        if (move_direction == move_direction::UP) {
            dir = model::Direction::NORTH;
            speed = {0.0, -default_speed};
        }
        else if (move_direction == move_direction::DOWN) {
            dir = model::Direction::SOUTH;
            speed = {0.0, default_speed};
        }
        else if (move_direction == move_direction::LEFT) {
            dir = model::Direction::WEST;
            speed = {-default_speed, 0.0};
        }
        else if (move_direction == move_direction::RIGHT) {
            dir = model::Direction::EAST;
            speed = {default_speed, 0.0};
        }
        else {
            dir = model::Direction::NONE;
            speed = {0.0, 0.0};
        }

        player->GetDog()->SetDefaultSpeed(default_speed);
        player->GetDog()->SetSpeed(speed);
        player->GetDog()->SetDirection(dir);
    }


    GameTickUseCase::GameTickUseCase(model::Game& game, Players& players, postgres_database::DataBase& game_db) 
        : game_(game), players_(players), game_db_(game_db) {}

    void GameTickUseCase::UpdateState(std::chrono::milliseconds time) {
        for (auto [map_id, session] : game_.GetSessions()) {
            if (session) {
                auto inactive_dogs = session->UpdateState(time);

                for (const auto& dog : inactive_dogs) {
                    game_db_.SaveRetiredPlayer({ dog->GetName(), static_cast<int>(dog->GetScore()), dog->GetLeaveTime() });
                    session->DeletePlayer(dog);
                    players_.DeletePlayer(dog->GetId());
                }
            }
        }
    }


    RecordsUseCase::RecordsUseCase(const postgres_database::DataBase& game_db) : game_db_(game_db) {}

    std::vector<domain::RetiredPlayer> RecordsUseCase::GetRecords(int offset, int max_elements) const {
        return game_db_.GetRetiredPlayers(offset, max_elements);
    }


    Application::Application(model::Game& game, Players& players, postgres_database::DataBaseConfig db_config) : 
        game_(game),
        game_db_(db_config),
        players_(players),
        list_maps_(game),
        get_map_(game),
        join_game_(game, players, randomize_spavn_dogs_),
        list_players_(game, players),
        game_state_(players),
        player_state_action_(players),
        game_tick_(game, players, game_db_),
        records_(game_db_) {}

    Players& Application::GetPlayers() {
        return players_;
    }

    const model::Game::Maps& Application::ListMaps() const {
        return list_maps_.GetList();
    }

    const model::Map* Application::FindMap(std::string_view map_id_str) const {
        return get_map_.Find(map_id_str);
    }

    const JoinGameUseCase::JoinGameResult Application::JoinGame(std::string_view map_id_str, std::string_view name_str) {
        try{
            return join_game_.Join(std::string(map_id_str), std::string(name_str));
        }
        catch(const ApiError& error) {
            throw error;
        }
    }

    const model::GameSession::Dogs& Application::ListPlayers(const Token& token) const {
        return list_players_.List(token);
    }

    const model::GameSession::GameStateData Application::GameState(const Token& token) const {
        try{
            return game_state_.GetState(token);
        }
        catch(const ApiError& error) {
            throw error;
        }
    }

    void Application::SetPlayerAction(const Token& token, std::string_view move_direction) {
        player_state_action_.SetAction(token, move_direction);
    }

    bool Application::IsAutoTickEnabled() const {
        return auto_tick_enabled_;
    }

    void Application::SetAutoTickEnabled(bool enabled) {
        auto_tick_enabled_ = enabled;
    }

    void Application::Tick(std::chrono::milliseconds delta) {
        game_tick_.UpdateState(delta);
        if (listener_) {
            listener_->OnTick(delta);
        }
    }

    void Application::SetGenerateRandPos(bool enabled) {
        randomize_spavn_dogs_ = enabled;
    }

    void Application::SetApplicationListener(std::unique_ptr<ApplicationListener> listener) {
        listener_ = std::move(listener);
    }

    const std::vector<domain::RetiredPlayer> Application::Records(int offset, int max_elements) const {
        return records_.GetRecords(offset, max_elements);
    }
    
}