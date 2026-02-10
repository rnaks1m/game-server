#pragma once

#include "model.h"
#include "retired_player.h"

#include <iostream>
#include <condition_variable>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/pqxx>
#include <vector>

namespace postgres_database {

using pqxx::operator"" _zv;

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

struct DataBaseConfig {
    std::string db_url;
    size_t pool_capacity = 4;
};

DataBaseConfig GetConfigFromEnv();

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
    }

    ConnectionWrapper GetConnection() {
        std::unique_lock lock{mutex_};
        // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
        // хотя бы одно соединение
        cond_var_.wait(lock, [this] {
            return used_connections_ < pool_.size();
        });
        // После выхода из цикла ожидания мьютекс остаётся захваченным

        return {std::move(pool_[used_connections_++]), *this};
    }

private:
    void ReturnConnection(ConnectionPtr&& conn) {
        // Возвращаем соединение обратно в пул
        {
            std::lock_guard lock{mutex_};
            assert(used_connections_ != 0);
            pool_[--used_connections_] = std::move(conn);
        }
        // Уведомляем один из ожидающих потоков об изменении состояния пула
        cond_var_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};



class RetiredPlayerRepositoryImpl : public domain::RetiredPlayerRepository {
public:
    explicit RetiredPlayerRepositoryImpl(ConnectionPool& conn_pool);

    void Save(const domain::RetiredPlayer& player) override;
    std::vector<domain::RetiredPlayer> LoadFromDB(int offset, int max_elem) const override;

private:
    ConnectionPool& conn_pool_;
};



class DataBase {
public:
    explicit DataBase(const DataBaseConfig& config);

    void SaveRetiredPlayer(const model::RetiredPlayersInfo& player);
    const std::vector<domain::RetiredPlayer> GetRetiredPlayers(int offset, int max_elem) const;
    
private:
    ConnectionPool conn_pool_;
    RetiredPlayerRepositoryImpl players_rep_;
};


} // namespace postgres_database