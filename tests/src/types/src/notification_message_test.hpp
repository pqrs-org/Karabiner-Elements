#include "test.hpp"
#include "types.hpp"
#include <boost/ut.hpp>

void run_notification_message_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "notification_message"_test = [] {
    {
      auto json = nlohmann::json::object({
          {"id", "notification_message_id"},
          {"text", "notification_message_text"},
      });

      auto value = json.get<krbn::notification_message>();
      expect(value.get_id() == "notification_message_id");
      expect(value.get_text() == "notification_message_text");
      expect(value.get_duration_milliseconds() == std::chrono::milliseconds(0));

      expect(nlohmann::json(value) == json);
    }
    {
      auto json = nlohmann::json::object({
          {"id", "notification_message_id"},
          {"text", "notification_message_text"},
          {"duration_milliseconds", 3000},
      });

      auto value = json.get<krbn::notification_message>();
      expect(value.get_id() == "notification_message_id");
      expect(value.get_text() == "notification_message_text");
      expect(value.get_duration_milliseconds() == std::chrono::milliseconds(3000));

      expect(nlohmann::json(value) == json);
    }
    {
      auto json = nlohmann::json::object({
          {"id", "notification_message_id"},
          {"text", "notification_message_text"},
          {"duration_milliseconds", 0},
      });

      auto value = json.get<krbn::notification_message>();
      expect(value.get_duration_milliseconds() == std::chrono::milliseconds(0));

      json.erase("duration_milliseconds");
      expect(nlohmann::json(value) == json);
    }

    json_unmarshal_error_test<krbn::notification_message>(
        nlohmann::json::object({
            {"duration_milliseconds", nullptr},
        }),
        "`duration_milliseconds` must be number, but is `null`");

    json_unmarshal_error_test<krbn::notification_message>(
        nlohmann::json::object({
            {"duration_milliseconds", -1},
        }),
        "`duration_milliseconds` must be greater than or equal to 0, but is `-1`");
  };
}
