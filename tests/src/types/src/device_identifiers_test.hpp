#include "../../share/ut_helper.hpp"
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
      expect(false == di.get_is_consumer());
      expect(false == di.get_is_virtual_device());
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
      expect(false == di.get_is_consumer());
      expect(false == di.get_is_virtual_device());
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
    {
      auto json = nlohmann::json::object({
          {"is_consumer", true},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(true == di.get_is_consumer());
      expect(true == nlohmann::json(di).at("is_consumer").get<bool>());
      expect(false == di.empty());
    }
    // is_virtual_device
    {
      auto json0 = R"(

{
  "is_virtual_device": true
}

)"_json;

      auto json1 = R"(

{
  "vendor_id": 1234,
  "product_id": 5678,
  "is_keyboard": true,
  "is_virtual_device": true
}

)"_json;

      auto json2 = R"(

{
  "vendor_id": 1234,
  "product_id": 5678,
  "is_keyboard": true
}

)"_json;

      auto di0 = json0.get<krbn::device_identifiers>();
      auto di1 = json1.get<krbn::device_identifiers>();
      auto di2 = json2.get<krbn::device_identifiers>();
      expect(true == di0.get_is_virtual_device());
      expect(true == di1.get_is_virtual_device());
      expect(false == di2.get_is_virtual_device());
      expect(false == di0.empty());
      expect(di1 != di2);
      expect(json0 == nlohmann::json(di0)) << UT_SHOW_LINE;
      expect(json1 == nlohmann::json(di1)) << UT_SHOW_LINE;
      expect(json2 == nlohmann::json(di2)) << UT_SHOW_LINE;
    }
  };
}
