#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"

#include <cmath>
#include <sstream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>


namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
  static std::string convert(collision_detector::GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

      return tmp.str();
  }
};
}  // namespace Catch


using namespace Catch::Matchers;

class TestProvider : public collision_detector::ItemGathererProvider {
public:
    TestProvider(std::vector<collision_detector::Item> items, std::vector<collision_detector::Gatherer> gatherers) :
        items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

bool EventsEqual(const collision_detector::GatheringEvent& a, const collision_detector::GatheringEvent& b) {
    return a.item_id == b.item_id &&
           a.gatherer_id == b.gatherer_id &&
           std::abs(a.sq_distance - b.sq_distance) < 1e-10 &&
           std::abs(a.time - b.time) < 1e-10;
}

class GatheringEventMatcher : public Catch::Matchers::MatcherGenericBase {
public:
    GatheringEventMatcher(size_t gatherer_id, size_t item_id, double expected_time, double expected_sq_distance)
        : gatherer_id_(gatherer_id), item_id_(item_id), 
          expected_time_(expected_time), expected_sq_distance_(expected_sq_distance) {}

    bool match(const collision_detector::GatheringEvent& event) const {
        return event.gatherer_id == gatherer_id_ &&
               event.item_id == item_id_ &&
               WithinRel(expected_time_, 1e-10).match(event.time) &&
               WithinRel(expected_sq_distance_, 1e-10).match(event.sq_distance);
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "has gatherer_id: " << gatherer_id_ 
           << ", item_id: " << item_id_
           << ", time: " << expected_time_
           << ", sq_distance: " << expected_sq_distance_;
        return ss.str();
    }

private:
    size_t gatherer_id_;
    size_t item_id_;
    double expected_time_;
    double expected_sq_distance_;
};

// Функция для удобного создания матчера события
GatheringEventMatcher IsGatheringEvent(size_t gatherer_id, size_t item_id, 
                                       double time, double sq_distance) {
    return GatheringEventMatcher(gatherer_id, item_id, time, sq_distance);
}

// Предикат для проверки хронологического порядка
auto IsInChronologicalOrder = [](const std::vector<collision_detector::GatheringEvent>& events) {
    for (size_t i = 1; i < events.size(); ++i) {
        if (events[i].time < events[i-1].time) {
            return false;
        }
    }
    return true;
};

// ============================================================================
// ТЕСТ 1: Детектирует все события столкновений
// ============================================================================
TEST_CASE("Detects all collision events") {
    // Два собирателя и два предмета - должно быть 2 события
    collision_detector::Gatherer gatherer1{geom::Point2D{0.0, 0.0}, geom::Point2D{2.0, 0.0}, 0.6};
    collision_detector::Gatherer gatherer2{geom::Point2D{0.0, 2.0}, geom::Point2D{2.0, 2.0}, 0.6};
    collision_detector::Item item1{geom::Point2D{1.0, 0.0}, 0.3};
    collision_detector::Item item2{geom::Point2D{1.0, 2.0}, 0.3};
    
    TestProvider provider({item1, item2}, {gatherer1, gatherer2});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
}

// ============================================================================
// ТЕСТ 2: Не детектирует лишних событий
// ============================================================================
TEST_CASE("Does not detect extra events 2") {
    // Собиратель движется мимо предмета (не достает) - событий быть не должно
    collision_detector::Gatherer gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{2.0, 0.0}, 0.6};
    collision_detector::Item item1{geom::Point2D{1.0, 0.0}, 0.3};
    collision_detector::Item item2{geom::Point2D{1.0, 1.0}, 0.3};
    
    TestProvider provider({item1, item2}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
}

// ============================================================================
// ТЕСТ 3: События идут в хронологическом порядке
// ============================================================================
TEST_CASE("Events are in chronological order") {
    // Два предмета на пути собирателя - события должны быть упорядочены по времени
    collision_detector::Gatherer gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{4.0, 0.0}, 0.6};
    collision_detector::Item item1{geom::Point2D{1.0, 0.0}, 0.3};
    collision_detector::Item item2{geom::Point2D{3.0, 0.0}, 0.3};
    
    TestProvider provider({item1, item2}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
    CHECK(IsInChronologicalOrder(events));
}

// ============================================================================
// ТЕСТ 4: События имеют правильные данные
// ============================================================================
TEST_CASE("Events have correct data") {
    // Один собиратель и один предмет - проверяем точные параметры события
    collision_detector::Gatherer gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{2.0, 0.0}, 0.6};
    collision_detector::Item item{geom::Point2D{1.0, 0.0}, 0.3};
    
    TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    
    const auto& event = events[0];
    
    // Проверяем корректные индексы
    CHECK(event.gatherer_id == 0);
    CHECK(event.item_id == 0);
    
    // Проверяем корректное время (0.5 - середина пути)
    CHECK_THAT(event.time, WithinRel(0.5, 1e-10));
    
    // Проверяем корректное расстояние (0 - предмет прямо на пути)
    CHECK_THAT(event.sq_distance, WithinRel(0.0, 1e-10));
    
    // Проверяем допустимые диапазоны
    CHECK(event.time >= 0.0);
    CHECK(event.time <= 1.0);
    CHECK(event.sq_distance >= 0.0);
}