#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "file_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("read_file") {
  {
    auto buffer = krbn::file_utility::read_file("data/not_found");

    REQUIRE(!buffer);
  }

  {
    auto buffer = krbn::file_utility::read_file("data/app.icns");

    REQUIRE(buffer);
    REQUIRE(buffer->size() == 225321);
    REQUIRE((*buffer)[0] == 0x69);
    REQUIRE((*buffer)[1] == 0x63);
    REQUIRE((*buffer)[2] == 0x6e);
    REQUIRE((*buffer)[3] == 0x73);
    REQUIRE((*buffer)[4] == 0x00);
    REQUIRE((*buffer)[5] == 0x03);
  }
}
