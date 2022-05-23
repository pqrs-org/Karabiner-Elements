#include "types.hpp"
#include <boost/ut.hpp>

void run_notification_message_test(void) {
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

      expect(nlohmann::json(value) == json);
    }
  };
}
