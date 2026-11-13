#include "connected_devices.hpp"
#include "json_utility.hpp"
#include <boost/ut.hpp>

void run_connected_devices_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "connected_devices"_test = [] {
    {
      krbn::connected_devices connected_devices;
      expect(nlohmann::json(connected_devices) == nlohmann::json::array());
    }
    {
      krbn::connected_devices connected_devices;

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

      expect(connected_devices.get_devices().size() == 6);
      expect(connected_devices.get_devices()[0].properties->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[0].properties->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(connected_devices.get_devices()[0].properties->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[1].properties->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(connected_devices.get_devices()[1].properties->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5679));
      expect(connected_devices.get_devices()[0].properties->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[2].properties->get_device_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(2345));
      expect(connected_devices.get_devices()[2].properties->get_device_identifiers().get_product_id() == pqrs::hid::product_id::value_t(6789));
      expect(connected_devices.get_devices()[0].properties->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[4].properties->get_device_identifiers().get_device_address() == "");
      expect(connected_devices.get_devices()[5].properties->get_device_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");

      std::ifstream ifs("json/connected_devices.json");

      expect(krbn::json_utility::parse_jsonc(ifs) == nlohmann::json(connected_devices));
    }

    // Additional identifier fields distinguish devices with the same VID/PID.
    {
      krbn::connected_devices connected_devices;

      for (const auto& address : {"aa-bb-cc-dd-ee-01", "aa-bb-cc-dd-ee-02"}) {
        connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
            .vendor_id = pqrs::hid::vendor_id::value_t(1234),
            .product_id = pqrs::hid::product_id::value_t(5678),
            .device_address = address,
            .is_keyboard = true,
            .additional_device_identifier_fields = {"device_address"},
        }));
      }

      expect(connected_devices.get_devices().size() == 2);
    }

    // Physical properties are aggregated when multiple devices have the same identifiers.
    {
      krbn::connected_devices connected_devices;

      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .location_id = krbn::location_id(200),
          .serial_number = "serial2",
          .transport = "Bluetooth",
          .device_address = "aa-bb-cc-dd-ee-02",
          .is_keyboard = true,
      }));
      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .location_id = krbn::location_id(100),
          .serial_number = "serial1",
          .transport = "USB",
          .device_address = "aa-bb-cc-dd-ee-01",
          .is_keyboard = true,
      }));
      connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .location_id = krbn::location_id(200),
          .serial_number = "serial2",
          .transport = "Bluetooth",
          .device_address = "aa-bb-cc-dd-ee-02",
          .is_keyboard = true,
      }));

      expect(connected_devices.get_devices().size() == 1);

      const auto& device = connected_devices.get_devices()[0];
      const auto& values = device.get_device_identifier_values_for_ui().get_values();
      expect(values == std::vector<krbn::connected_device::device_identifier_value_for_ui>{
                           {
                               .location_id = krbn::location_id(100),
                               .device_address = "aa-bb-cc-dd-ee-01",
                               .serial_number = "serial1",
                               .transport = "USB",
                           },
                           {
                               .location_id = krbn::location_id(200),
                               .device_address = "aa-bb-cc-dd-ee-02",
                               .serial_number = "serial2",
                               .transport = "Bluetooth",
                           },
                       });

      auto restored = nlohmann::json(connected_devices).get<krbn::connected_devices>();
      const auto& restored_values = restored.get_devices()[0].get_device_identifier_values_for_ui().get_values();
      expect(restored_values == values);
    }

    {
      std::ifstream ifs("json/connected_devices.json");
      auto expected = krbn::connected_devices(krbn::json_utility::parse_jsonc(ifs));

      expect(expected.get_devices().size() == 6);
    }
  };

  "connected device capabilities"_test = [] {
    krbn::connected_device_capabilities capabilities;
    capabilities.set_logitech_hidpp_divertable_control_ids({82, 196});

    krbn::connected_devices connected_devices;
    connected_devices.push_back_device(
        std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
            .device_id = krbn::device_id(1),
            .vendor_id = pqrs::hid::vendor_id::value_t(0x046d),
            .product_id = pqrs::hid::product_id::value_t(0xb037),
            .is_pointing_device = true,
        }),
        capabilities);

    auto json = nlohmann::json(connected_devices);
    expect(json.at(0).at("connected_device_capabilities") == nlohmann::json({
                                                                 {"logitech_hidpp",
                                                                  {
                                                                      {"divertable_control_ids", {82, 196}},
                                                                  }},
                                                             }));

    auto restored = json.get<krbn::connected_devices>();
    expect(restored.get_devices()[0].capabilities.get_logitech_hidpp_divertable_control_ids() ==
           std::vector<uint16_t>{82, 196});
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

    krbn::connected_devices connected_devices;

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
        nlohmann::json(connected_devices).dump();
        expect(false);
      } catch (nlohmann::detail::type_error& ex) {
        expect(std::string_view("[json.exception.type_error.316] invalid UTF-8 byte at index 5: 0x80") == ex.what());
      } catch (...) {
        expect(false);
      }

      std::ifstream ifs("json/ill_formed_name_expected.json");
      auto expected = krbn::json_utility::parse_jsonc(ifs);

      expect(krbn::json_utility::dump(nlohmann::json(connected_devices)) == krbn::json_utility::dump(expected));
    }
  };
}
