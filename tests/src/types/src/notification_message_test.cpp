#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("notification_message") {
  {
    auto json = nlohmann::json::object({
        {"id", "notification_message_id"},
        {"text", "notification_message_text"},
    });

    auto value = json.get<krbn::notification_message>();
    REQUIRE(value.get_id() == "notification_message_id");
    REQUIRE(value.get_text() == "notification_message_text");

    REQUIRE(nlohmann::json(value) == json);
  }
}
