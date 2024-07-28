#include "types.hpp"
#include <boost/ut.hpp>

void run_device_identifiers_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device_identifiers"_test = [] {
    {
      auto json = nlohmann::json::object();
      auto di = json.get<krbn::device_identifiers>();

      expect(pqrs::hid::vendor_id::value_t(0) == di.get_vendor_id());
      expect(pqrs::hid::product_id::value_t(0) == di.get_product_id());
      expect("" == di.get_device_address());
      expect(false == di.get_is_keyboard());
      expect(false == di.get_is_pointing_device());
      expect(false == di.get_is_game_pad());
      expect(true == di.empty());
    }
    {
      auto json = nlohmann::json::object({
          {"vendor_id", 1234},
          {"product_id", 5678},
          {"device_address", "aa-bb-cc-dd-ee-ff"},
          {"is_keyboard", true},
          {"dummy key", "dummy value"},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(pqrs::hid::vendor_id::value_t(1234) == di.get_vendor_id());
      expect(pqrs::hid::product_id::value_t(5678) == di.get_product_id());
      expect("aa-bb-cc-dd-ee-ff" == di.get_device_address());
      expect(true == di.get_is_keyboard());
      expect(false == di.get_is_pointing_device());
      expect(false == di.get_is_game_pad());
      expect(false == di.empty());
      expect(json == nlohmann::json(di));
    }
    {
      auto json = nlohmann::json::object({
          {"is_pointing_device", true},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(true == di.get_is_pointing_device());
      expect(true == nlohmann::json(di).at("is_pointing_device").get<bool>());
      expect(false == di.empty());
    }
    {
      auto json = nlohmann::json::object({
          {"is_game_pad", true},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(true == di.get_is_game_pad());
      expect(true == nlohmann::json(di).at("is_game_pad").get<bool>());
      expect(false == di.empty());
    }
  };
}
