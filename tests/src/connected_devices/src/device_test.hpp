#include "../../share/ut_helper.hpp"
#include "connected_devices/connected_devices.hpp"
#include <boost/ut.hpp>

void run_device_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "connected_devices::details::device"_test = [] {
    {
      krbn::connected_devices::details::descriptions descriptions(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                  pqrs::hid::product_string::value_t("product1"),
                                                                  "USB");
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                           pqrs::hid::product_id::value_t(5678),
                                           true,  // is_keyboard
                                           false, // is_pointing_device
                                           false, // is_game_pad
                                           false, // is_virtual_device
                                           ""     // device_address
      );
      krbn::connected_devices::details::device device(descriptions,
                                                      identifiers,
                                                      true,
                                                      true,
                                                      true);

      expect(device.get_descriptions() == descriptions);
      expect(device.get_identifiers() == identifiers);
      expect(device.get_is_built_in_keyboard() == true);
      expect(device.get_is_built_in_trackpad() == true);
      expect(device.get_is_built_in_touch_bar() == true);

      auto expected_json = R"(

{
  "descriptions": {
    "manufacturer": "manufacturer1",
    "product": "product1",
    "transport": "USB"
  },
  "identifiers": {
    "vendor_id": 1234,
    "product_id": 5678,
    "is_keyboard": true
  },
  "is_built_in_keyboard": true,
  "is_built_in_trackpad": true,
  "is_built_in_touch_bar": true
}

)"_json;

      expect(expected_json == device.to_json()) << UT_SHOW_LINE;
    }
    {
      auto device1 = krbn::connected_devices::details::device(nlohmann::json(nullptr));
      auto device2 = krbn::connected_devices::details::device(R"(

{
  "descriptions": {
    "manufacturer": "manufacturer2",
    "product": "product2"
  },
  "identifiers": {
    "vendor_id": 1234,
    "product_id": 5678,
    "is_keyboard": true,
    "is_pointing_device": false
  },
  "is_built_in_keyboard": true,
  "is_built_in_trackpad": true,
  "is_built_in_touch_bar": true
}

)"_json);

      expect(device1.get_descriptions().get_manufacturer() == pqrs::hid::manufacturer_string::value_t(""));
      expect(device1.get_descriptions().get_product() == pqrs::hid::product_string::value_t(""));
      expect(device1.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(device1.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(device1.get_identifiers().get_device_address() == "");
      expect(device1.get_identifiers().get_is_keyboard() == false);
      expect(device1.get_identifiers().get_is_pointing_device() == false);
      expect(device1.get_is_built_in_keyboard() == false);
      expect(device1.get_is_built_in_trackpad() == false);
      expect(device1.get_is_built_in_touch_bar() == false);

      expect(device2.get_descriptions().get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer2"));
      expect(device2.get_descriptions().get_product() == pqrs::hid::product_string::value_t("product2"));
      expect(device2.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(device2.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(device2.get_identifiers().get_is_keyboard() == true);
      expect(device2.get_identifiers().get_is_pointing_device() == false);
      expect(device2.get_is_built_in_keyboard() == true);
      expect(device2.get_is_built_in_trackpad() == true);
      expect(device2.get_is_built_in_touch_bar() == true);
    }

    // from device_properties

    {
      krbn::device_properties device_properties;
      krbn::connected_devices::details::device device(device_properties);

      expect(device.get_descriptions().get_manufacturer() == pqrs::hid::manufacturer_string::value_t(""));
      expect(device.get_descriptions().get_product() == pqrs::hid::product_string::value_t(""));
      expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(device.get_identifiers().get_device_address() == "");
      expect(device.get_identifiers().get_is_keyboard() == false);
      expect(device.get_identifiers().get_is_pointing_device() == false);
      expect(device.get_is_built_in_keyboard() == false);
      expect(device.get_is_built_in_trackpad() == false);
      expect(device.get_is_built_in_touch_bar() == false);
    }
    {
      krbn::device_properties device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer"),
          .product = pqrs::hid::product_string::value_t("product"),
          .device_address = "ec-ba-73-21-e6-f4", // ignored
          .is_keyboard = true,
      });

      {
        krbn::connected_devices::details::device device(device_properties);

        expect(device.get_descriptions().get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer"));
        expect(device.get_descriptions().get_product() == pqrs::hid::product_string::value_t("product"));
        expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect(device.get_identifiers().get_device_address() == "");
        expect(device.get_identifiers().get_is_keyboard() == true);
        expect(device.get_identifiers().get_is_pointing_device() == false);
        expect(device.get_is_built_in_keyboard() == false);
        expect(device.get_is_built_in_trackpad() == false);
        expect(device.get_is_built_in_touch_bar() == false);
      }
    }
    {
      krbn::device_properties device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0),
          .product_id = pqrs::hid::product_id::value_t(0),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer"),
          .product = pqrs::hid::product_string::value_t("product"),
          .device_address = "ec-ba-73-21-e6-f5",
          .is_keyboard = true,
      });

      {
        krbn::connected_devices::details::device device(device_properties);

        expect(device.get_descriptions().get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer"));
        expect(device.get_descriptions().get_product() == pqrs::hid::product_string::value_t("product"));
        expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
        expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
        expect(device.get_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");
        expect(device.get_identifiers().get_is_keyboard() == true);
        expect(device.get_identifiers().get_is_pointing_device() == false);
        expect(device.get_is_built_in_keyboard() == false);
        expect(device.get_is_built_in_trackpad() == false);
        expect(device.get_is_built_in_touch_bar() == false);
      }
    }

    // is_apple
    {
      krbn::device_properties device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1234),
          .product_id = pqrs::hid::product_id::value_t(5678),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer"),
          .product = pqrs::hid::product_string::value_t("product"),
          .is_keyboard = true,
      });
      expect(false == krbn::connected_devices::details::device(device_properties).is_apple());
    }
    {
      krbn::device_properties device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1452),
          .product_id = pqrs::hid::product_id::value_t(610),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("manufacturer"),
          .product = pqrs::hid::product_string::value_t("product"),
          .is_keyboard = true,
      });
      expect(true == krbn::connected_devices::details::device(device_properties).is_apple());
    }
    {
      krbn::device_properties device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0),
          .product_id = pqrs::hid::product_id::value_t(0),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("Apple"),
          .product = pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad"),
          .is_keyboard = true,
      });
      expect(true == krbn::connected_devices::details::device(device_properties).is_apple());
    }
  };
}
