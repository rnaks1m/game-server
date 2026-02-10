#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/model.h"
#include "../src/model_serialization.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, {42.2, 12.5}, 3};
            dog.IncreaseScore(42);
            CHECK(dog.AddItemToBag(Loot::Id{10}, 2u));
            dog.SetDirection(Direction::EAST);
            dog.SetSpeed({2.3, -1.2});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                auto dog_pos = dog.GetPosition();
                auto res_pos = restored.GetPosition();
                auto dog_speed = dog.GetSpeed();
                auto res_speed = restored.GetSpeed();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog_pos.x == res_pos.x);
                CHECK(dog_pos.y == res_pos.y);
                CHECK(dog_speed.x == res_speed.x);
                CHECK(dog_speed.y == res_speed.y);
                CHECK(dog.GetBagCapacity() == restored.GetBagCapacity());
                
                const auto& original_items = dog.GetItemsFromBag();
                const auto& restored_items = restored.GetItemsFromBag();
                CHECK(original_items.size() == restored_items.size());
                if (original_items.size() == restored_items.size()) {
                    for (size_t i = 0; i < original_items.size(); ++i) {
                        CHECK(original_items[i].id == restored_items[i].id);
                        CHECK(original_items[i].type == restored_items[i].type);
                    }
                }
            }
        }
    }
}
