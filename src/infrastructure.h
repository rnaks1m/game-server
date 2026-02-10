#pragma once

#include "app_serialization.h"

namespace infrastructure {

using namespace std::literals;

class SerializingListener : public::app::ApplicationListener {
public:
    explicit SerializingListener(app::Application& app, std::chrono::milliseconds saving_interval) :
        app_(app), saving_interval_(saving_interval), time_after_saving_(0ms) {}

    void OnTick(std::chrono::milliseconds time) override {
        time_after_saving_ += time;
        if (time_after_saving_ >= saving_interval_) {
            serialization::AppSerialization(file_to_serialize_, app_);
            time_after_saving_ = 0ms;
        }
    }

    void SetSerializeFile(const std::filesystem::path& file_to_serialize) {
        file_to_serialize_ = file_to_serialize;
    }

private:
    app::Application& app_;
    std::chrono::milliseconds saving_interval_;
    std::chrono::milliseconds time_after_saving_;
    std::filesystem::path file_to_serialize_;
};

}