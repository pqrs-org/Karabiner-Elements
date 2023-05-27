#include "types.hpp"
#include <boost/ut.hpp>

void run_device_identifiers_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device_identifiers"_test = [] {
    {
      krbn::device_identifiers di(pqrs::hid::vendor_id::value_t(1234),
                                  pqrs::hid::product_id::value_t(5678),
                                  true,  // is_keyboard
                                  false, // is_pointing_device
                                  ""     // device_address
      );
      expect(di.is_apple() == false);
    }
    {
      krbn::device_identifiers di(pqrs::hid::vendor_id::value_t(1452),
                                  pqrs::hid::product_id::value_t(610),
                                  true,  // is_keyboard
                                  false, // is_pointing_device
                                  ""     // device_address
      );
      expect(di.is_apple() == true);
    }
    {
      auto json = nlohmann::json::object();
      auto di = json.get<krbn::device_identifiers>();
      expect(di.get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(di.get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(di.get_device_address() == "");
      expect(di.get_is_keyboard() == false);
      expect(di.get_is_pointing_device() == false);
    }
    {
      auto json = nlohmann::json::object({
          {"vendor_id", 1234},
          {"product_id", 5678},
          {"device_address", "aa-bb-cc-dd-ee-ff"},
          {"is_keyboard", true},
          {"is_pointing_device", false},
          {"dummy key", "dummy value"},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(di.get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(di.get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(di.get_device_address() == "aa-bb-cc-dd-ee-ff");
      expect(di.get_is_keyboard() == true);
      expect(di.get_is_pointing_device() == false);
      expect(nlohmann::json(di) == json);
    }
    {
      auto json = nlohmann::json::object({
          {"is_pointing_device", true},
      });

      auto di = json.get<krbn::device_identifiers>();
      expect(di.get_is_pointing_device() == true);
    }
  };
}
