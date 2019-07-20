#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "state_json_writer.hpp"

TEST_CASE("state_json_writer") {
  system("rm -rf tmp");
  system("mkdir -p tmp");

  {
    krbn::state_json_writer writer("tmp/state.json");

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/empty_object.json"));

    writer.set("key1", 123);

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));

    writer.set("key1", 345);

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_2.json"));

    writer.set("key2", "value2");

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key2.json"));
  }
}
