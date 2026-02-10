#include "retired_player.h"

namespace domain {

RetiredPlayer::RetiredPlayer(RetiredPlayerId id, std::string name, int score, int play_time_ms)
    : id_(std::move(id)), name_(std::move(name)), score_(score), play_time_ms_(play_time_ms) {}

const RetiredPlayerId RetiredPlayer::GetId() const noexcept {
    return id_;
}

const std::string RetiredPlayer::GetName() const noexcept {
    return name_;
}

const int RetiredPlayer::GetScore() const noexcept {
    return score_;
}

const int RetiredPlayer::GetTimeMs() const noexcept {
    return play_time_ms_;
}

}