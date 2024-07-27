#include "connected_devices/connected_devices.hpp"
#include "json_utility.hpp"
#include <boost/ut.hpp>

void run_connected_devices_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "connected_devices"_test = [] {
    {
      krbn::connected_devices::connected_devices connected_devices;
      expect(connected_devices.to_json() == nlohmann::json::array());
    }
    {
      krbn::connected_devices::connected_devices connected_devices;

      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                    pqrs::hid::product_string::value_t("product1"),
                                                                    "USB");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                             pqrs::hid::product_id::value_t(5678),
                                             true,  // is_keyboard
                                             false, // is_pointing_device
                                             false, // is_game_pad
                                             ""     // device_address
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 true,  // is_built_in_keyboard
                                                                                 false, // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1 (ignored)"),
                                                                    pqrs::hid::product_string::value_t("product1 (ignored)"),
                                                                    "USB");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                             pqrs::hid::product_id::value_t(5678),
                                             true,  // is_keyboard
                                             false, // is_pointing_device
                                             false, // is_game_pad
                                             ""     // device_address
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 true,  // is_built_in_keyboard
                                                                                 false, // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer3"),
                                                                    pqrs::hid::product_string::value_t("product3"),
                                                                    "USB");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(2345),
                                             pqrs::hid::product_id::value_t(6789),
                                             false, // is_keyboard
                                             true,  // is_pointing_device
                                             false, // is_game_pad
                                             ""     // device_address
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 false, // is_built_in_keyboard
                                                                                 false, // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer2"),
                                                                    pqrs::hid::product_string::value_t("product2"),
                                                                    "USB");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                             pqrs::hid::product_id::value_t(5679),
                                             false, // is_keyboard
                                             true,  // is_pointing_device
                                             false, // is_game_pad
                                             ""     // device_address
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 false, // is_built_in_keyboard
                                                                                 true,  // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      // product4 is a combined device.
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                    pqrs::hid::product_string::value_t("product4"),
                                                                    "Bluetooth");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(123),
                                             pqrs::hid::product_id::value_t(678),
                                             false,              // is_keyboard
                                             true,               // is_pointing_device
                                             false,              // is_game_pad
                                             "ec-ba-73-21-e6-f4" // device_address (ignored)
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 false, // is_built_in_keyboard
                                                                                 true,  // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                    pqrs::hid::product_string::value_t("product4"),
                                                                    "Bluetooth");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(123),
                                             pqrs::hid::product_id::value_t(678),
                                             true,               // is_keyboard
                                             false,              // is_pointing_device
                                             false,              // is_game_pad
                                             "ec-ba-73-21-e6-f4" // device_address (ignored)
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 true,  // is_built_in_keyboard
                                                                                 false, // is_built_in_pointing_device
                                                                                 false  // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }
      // product5 is a bluetooth HID device that has no vendor_id/product_id
      {
        krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                    pqrs::hid::product_string::value_t("product5"),
                                                                    "Bluetooth");
        krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(0),
                                             pqrs::hid::product_id::value_t(0),
                                             true,               // is_keyboard
                                             false,              // is_pointing_device
                                             false,              // is_game_pad
                                             "ec-ba-73-21-e6-f5" // device_address
        );
        auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                                 identifiers,
                                                                                 false, // is_built_in_keyboard
                                                                                 false, // is_built_in_pointing_device
                                                                                 true   // is_built_in_touch_bar
        );
        connected_devices.push_back_device(device);
      }

      expect(connected_devices.is_loaded() == false);
      expect(connected_devices.get_devices().size() == 6);
      expect(connected_devices.get_devices()[0]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[0]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(connected_devices.get_devices()[0]->get_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[1]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[1]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5679));
      expect(connected_devices.get_devices()[0]->get_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[2]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(2345));
      expect(connected_devices.get_devices()[2]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(6789));
      expect(connected_devices.get_devices()[0]->get_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[4]->get_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[5]->get_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");

      std::ifstream ifs("json/connected_devices.json");

      expect(connected_devices.to_json() == krbn::json_utility::parse_jsonc(ifs));
    }

    {
      krbn::connected_devices::connected_devices connected_devices("json/connected_devices.json");

      expect(connected_devices.is_loaded() == true);
      expect(connected_devices.get_devices().size() == 6);
      expect(connected_devices.get_devices()[0]->get_is_built_in_keyboard() == true);
      expect(connected_devices.get_devices()[0]->get_is_built_in_trackpad() == false);
      expect(connected_devices.get_devices()[1]->get_is_built_in_keyboard() == false);
      expect(connected_devices.get_devices()[1]->get_is_built_in_trackpad() == true);
    }

    {
      krbn::connected_devices::connected_devices connected_devices("json/not_found.json");

      expect(connected_devices.is_loaded() == false);
      expect(connected_devices.get_devices().size() == 0);
    }

    {
      krbn::connected_devices::connected_devices connected_devices("json/broken.json");

      expect(connected_devices.is_loaded() == false);
      expect(connected_devices.get_devices().size() == 0);
    }
  };

  "connected_devices ill-formed name"_test = [] {
    const char ill_formed_name[] = {
        't',
        'e',
        's',
        't',
        ' ',
        static_cast<char>(0x80),
        '!',
        0,
    };

    krbn::connected_devices::connected_devices connected_devices;

    {
      krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t(ill_formed_name),
                                                                  pqrs::hid::product_string::value_t(ill_formed_name),
                                                                  "USB");
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                           pqrs::hid::product_id::value_t(5678),
                                           true,  // is_keyboard
                                           false, // is_pointing_device
                                           false, // is_game_pad
                                           ""     // device_address
      );
      auto device = std::make_shared<krbn::connected_devices::details::device>(descriptions,
                                                                               identifiers,
                                                                               true,  // is_built_in_keyboard
                                                                               false, // is_built_in_pointing_device
                                                                               false  // is_built_in_touch_bar
      );
      connected_devices.push_back_device(device);

      // Check throw with `dump`
      try {
        connected_devices.to_json().dump();
        expect(false);
      } catch (nlohmann::detail::type_error& ex) {
        expect(std::string_view("[json.exception.type_error.316] invalid UTF-8 byte at index 5: 0x80") == ex.what());
      } catch (...) {
        expect(false);
      }

      std::ifstream ifs("json/ill_formed_name_expected.json");
      auto expected = krbn::json_utility::parse_jsonc(ifs);

      expect(krbn::json_utility::dump(connected_devices.to_json()) == krbn::json_utility::dump(expected));
    }
  };
}
