#pragma once

#include "model.h"
#include "tagged_uuid.h"

namespace domain {

namespace detail {
struct RetiredPlayerTag {};
}  // namespace detail

using RetiredPlayerId = util::TaggedUUID<detail::RetiredPlayerTag>;

class RetiredPlayer {
public:
    RetiredPlayer(RetiredPlayerId id, std::string name, int score, int play_time_ms);

    const RetiredPlayerId GetId() const noexcept;
    const std::string GetName() const noexcept;
    const int GetScore() const noexcept;
    const int GetTimeMs() const noexcept;

private:
    RetiredPlayerId id_;
    std::string name_;
    int score_;
    int play_time_ms_;
};

class RetiredPlayerRepository {
public:
    virtual void Save(const RetiredPlayer& player) = 0;
    virtual std::vector<RetiredPlayer> LoadFromDB(int offset, int max_elem) const = 0;
protected:
    ~RetiredPlayerRepository() = default;
};
    
} // namespace domain