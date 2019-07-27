#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "state_json_writer.hpp"
#include <pqrs/filesystem.hpp>

TEST_CASE("state_json_writer") {
  system("rm -rf tmp");
  system("mkdir -p tmp");

  {
    krbn::state_json_writer writer("tmp/state.json");

    REQUIRE(!pqrs::filesystem::exists("tmp/state.json"));

    writer.set("key1", 123);

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));

    writer.set("key1", 345);

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_2.json"));

    writer.set("key2", "value123");

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key2_1.json"));
  }

  {
    krbn::state_json_writer writer("tmp/state.json");

    writer.set("key2", "value345");

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key2_2.json"));
  }

  {
    system("echo '[' > tmp/state.json");

    krbn::state_json_writer writer("tmp/state.json");

    writer.set("key1", 123);

    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));
  }
}
