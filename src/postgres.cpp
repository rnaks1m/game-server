#include "postgres.h"

namespace postgres_database {

using namespace std::literals;

DataBaseConfig GetConfigFromEnv() {
    DataBaseConfig config;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        config.db_url = url;
    }
    else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return config;
}



RetiredPlayerRepositoryImpl::RetiredPlayerRepositoryImpl(ConnectionPool& conn_pool) : conn_pool_(conn_pool) {}

void RetiredPlayerRepositoryImpl::Save(const domain::RetiredPlayer& player) {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work(*conn);
    std::string query_text = "INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);";
    work.exec_params(query_text, player.GetId().ToString(), player.GetName(), player.GetScore(), player.GetTimeMs());
    work.commit();
}

std::vector<domain::RetiredPlayer> RetiredPlayerRepositoryImpl::LoadFromDB(int offset, int max_elem) const {
    auto conn = conn_pool_.GetConnection();
    pqxx::read_transaction read{*conn};
    std::vector<domain::RetiredPlayer> result;
    std::string query_text = "SELECT id, name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name LIMIT " 
        + std::to_string(max_elem) + " OFFSET " + std::to_string(offset) + ";";

    for(const auto& [id, name, score, time] : read.query<std::string, std::string, int, int>(query_text)) {
        result.emplace_back(domain::RetiredPlayerId::FromString(id), name, score, time);
    }

    return result;
}

DataBase::DataBase(const DataBaseConfig& config)
    : conn_pool_(config.pool_capacity, [&db_url = config.db_url] { return std::make_shared<pqxx::connection>(db_url); })
    , players_rep_(conn_pool_) {

    auto conn = conn_pool_.GetConnection();
    pqxx::work work(*conn);

    work.exec(
    R"(CREATE TABLE IF NOT EXISTS retired_players (
        id UUID CONSTRAINT retired_player_id_constraint PRIMARY KEY,
        name varchar(100) NOT NULL,
        score integer,
        play_time_ms integer
    );)"_zv);

    work.exec(R"(CREATE INDEX IF NOT EXISTS retired_players_score_play_time_name_idx ON retired_players (score DESC, play_time_ms, name);)"_zv);

    work.commit();
}

void DataBase::SaveRetiredPlayer(const model::RetiredPlayersInfo& player) {
    players_rep_.Save(domain::RetiredPlayer{domain::RetiredPlayerId::New(), player.name, player.score, player.play_time});
}

const std::vector<domain::RetiredPlayer> DataBase::GetRetiredPlayers(int offset, int max_elem) const {
    return players_rep_.LoadFromDB(offset, max_elem);
}

}