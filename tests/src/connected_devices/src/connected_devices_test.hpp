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

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(1),
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer1"),
          .product = pqrs::hid::product_string::value_t("product1"),
          .transport = "USB",
          .is_keyboard = true,
      }));

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(2),
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer1 (ignored)"),
          .product = pqrs::hid::product_string::value_t("product1 (ignored)"),
          .transport = "USB",
          .is_keyboard = true,
      }));

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(3),
          .vendor_id = pqrs::hid::vendor_id::value_t(2345),
          .product_id = pqrs::hid::product_id::value_t(6789),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer3"),
          .product = pqrs::hid::product_string::value_t("product3"),
          .transport = "USB",
          .is_pointing_device = true,
      }));

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(4),
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5679),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer2"),
          .product = pqrs::hid::product_string::value_t("product2"),
          .transport = "USB",
          .is_pointing_device = true,
      }));

      //
      // product4 is a combined device.
      //

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(5),
          .vendor_id = pqrs::hid::vendor_id::value_t(123),
          .product_id = pqrs::hid::product_id::value_t(678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer1"),
          .product = pqrs::hid::product_string::value_t("product4"),
          .transport = "Bluetooth",
          .device_address = "ec-ba-73-21-e6-f4", // ignored
          .is_pointing_device = true,
      }));

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(6),
          .vendor_id = pqrs::hid::vendor_id::value_t(123),
          .product_id = pqrs::hid::product_id::value_t(678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer1"),
          .product = pqrs::hid::product_string::value_t("product4"),
          .transport = "Bluetooth",
          .device_address = "ec-ba-73-21-e6-f4", // ignored
          .is_keyboard = true,
      }));

      //
      // product5 is a bluetooth HID device that has no vendor_id/product_id
      //

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(7),
          .vendor_id = pqrs::hid::vendor_id::value_t(0),
          .product_id = pqrs::hid::product_id::value_t(0),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer1"),
          .product = pqrs::hid::product_string::value_t("product5"),
          .transport = "Bluetooth",
          .device_address = "ec-ba-73-21-e6-f5",
          .is_keyboard = true,
      }));

      expect(connected_devices.is_loaded() == false);
      expect(connected_devices.get_devices().size() == 6);
      expect(connected_devices.get_devices()[0]->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[0]->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(connected_devices.get_devices()[0]->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[1]->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[1]->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5679));
      expect(connected_devices.get_devices()[0]->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[2]->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(2345));
      expect(connected_devices.get_devices()[2]->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(6789));
      expect(connected_devices.get_devices()[0]->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[4]->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[5]->get_device_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");

      std::ifstream ifs("json/connected_devices.json");

      expect(krbn::json_utility::parse_jsonc(ifs) == connected_devices.to_json());
    }

    {
      krbn::connected_devices::connected_devices connected_devices("json/connected_devices.json");

      expect(connected_devices.is_loaded() == true);
      expect(connected_devices.get_devices().size() == 6);
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
      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(1),
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t(ill_formed_name),
          .product = pqrs::hid::product_string::value_t(ill_formed_name),
          .transport = "USB",
          .is_keyboard = true,
      }));

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
