#include "device_properties.hpp"
#include <boost/ut.hpp>

void run_device_properties_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device_properties"_test = [] {
    auto device_properties = krbn::device_properties::make_device_properties(krbn::device_id(42),
                                                                             nullptr);
    expect(krbn::device_id(42) == device_properties->get_device_id());
  };

  "device_properties.additional_device_identifier_fields"_test = [] {
    auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1234),
        .product_id = pqrs::hid::product_id::value_t(5678),
        .location_id = krbn::location_id(123456),
        .serial_number = "serial",
        .transport = "Bluetooth Low Energy",
        .device_address = "aa-bb-cc-dd-ee-ff",
        .is_keyboard = true,
    });

    expect(device_properties.get_device_identifiers().get_device_address().empty());
    expect(device_properties.get_device_identifiers().get_location_id() == krbn::location_id(0));
    expect(device_properties.get_device_identifiers().get_serial_number().empty());
    expect(device_properties.get_device_identifiers().get_transport().empty());

    device_properties.set_additional_device_identifier_fields({
        "device_address",
        "location_id",
        "serial_number",
        "transport",
    });
    expect(device_properties.get_device_identifiers().get_device_address() == "aa-bb-cc-dd-ee-ff");
    expect(device_properties.get_device_identifiers().get_location_id() == krbn::location_id(123456));
    expect(device_properties.get_device_identifiers().get_serial_number() == "serial");
    expect(device_properties.get_device_identifiers().get_transport() == "Bluetooth Low Energy");

    device_properties.set_additional_device_identifier_fields({});
    expect(device_properties.get_device_identifiers().get_device_address().empty());
    expect(device_properties.get_device_identifiers().get_location_id() == krbn::location_id(0));
    expect(device_properties.get_device_identifiers().get_serial_number().empty());
    expect(device_properties.get_device_identifiers().get_transport().empty());
  };

  "device_properties.is_virtual_device"_test = [] {
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{});
      expect(false == device_properties.get_device_identifiers().get_is_virtual_device());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .manufacturer = pqrs::hid::manufacturer_string::value_t("pqrs.org"),
          .product = pqrs::hid::product_string::value_t("Karabiner DriverKit VirtualHIDKeyboard x.y.z"),
      });
      expect(true == device_properties.get_device_identifiers().get_is_virtual_device());
    }
  };

  "device_properties.manufacturer_ overwrite"_test = [] {
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(1452),
          .product_id = pqrs::hid::product_id::value_t(34304),
      });
      expect(pqrs::hid::manufacturer_string::value_t("Apple Inc.") == device_properties.get_manufacturer());
      expect(pqrs::hid::product_string::value_t("Apple Internal Touch Bar") == device_properties.get_product());
    }
  };

  "device_properties.is_built_in_*"_test = [] {
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{});
      expect(false == device_properties.get_is_built_in_keyboard());
      expect(false == device_properties.get_is_built_in_pointing_device());
      expect(false == device_properties.get_is_built_in_touch_bar());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .product = pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad"),
          .is_keyboard = true,
      });
      expect(true == device_properties.get_is_built_in_keyboard());
      expect(false == device_properties.get_is_built_in_pointing_device());
      expect(false == device_properties.get_is_built_in_touch_bar());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .product = pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad"),
          .is_pointing_device = true,
      });
      expect(false == device_properties.get_is_built_in_keyboard());
      expect(true == device_properties.get_is_built_in_pointing_device());
      expect(false == device_properties.get_is_built_in_touch_bar());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .product = pqrs::hid::product_string::value_t("Apple Internal Touch Bar"),
          .is_keyboard = true,
      });
      expect(false == device_properties.get_is_built_in_keyboard());
      expect(false == device_properties.get_is_built_in_pointing_device());
      expect(true == device_properties.get_is_built_in_touch_bar());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .transport = "FIFO",
          .is_keyboard = true,
      });
      expect(true == device_properties.get_is_built_in_keyboard());
      expect(false == device_properties.get_is_built_in_pointing_device());
      expect(false == device_properties.get_is_built_in_touch_bar());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .transport = "FIFO",
          .is_pointing_device = true,
      });
      expect(false == device_properties.get_is_built_in_keyboard());
      expect(true == device_properties.get_is_built_in_pointing_device());
      expect(false == device_properties.get_is_built_in_touch_bar());
    }
  };

  "device_properties.is_apple"_test = [] {
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{});
      expect(false == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .product = pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad"),
          .is_keyboard = true,
      });
      expect(true == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .product = pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad"),
          .is_pointing_device = true,
      });
      expect(true == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0x05ac),
          .is_keyboard = true,
      });
      expect(true == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0x004c),
          .is_keyboard = true,
      });
      expect(true == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .transport = "FIFO",
          .is_pointing_device = true,
      });
      expect(true == device_properties.get_is_apple());
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0x05ac),
          .product_id = pqrs::hid::product_id::value_t(0x024f),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("pqrs.org"),
          .product = pqrs::hid::product_string::value_t("Karabiner DriverKit VirtualHIDKeyboard x.y.z"),
          .is_keyboard = true,
      });
      expect(false == device_properties.get_is_apple());
    }
  };
}
