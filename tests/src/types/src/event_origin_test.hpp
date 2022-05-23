#include "types.hpp"
#include <boost/ut.hpp>

void run_event_origin_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "event_origin"_test = [] {
    {
      auto event_origin = krbn::event_origin::grabbed_device;
      auto json = nlohmann::json(event_origin);
      expect(json.dump() == "\"grabbed_device\"");
    }
    {
      auto json = nlohmann::json("grabbed_device");
      expect(json.get<krbn::event_origin>() == krbn::event_origin::grabbed_device);
    }
  };
}
