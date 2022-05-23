#include "types.hpp"
#include <boost/ut.hpp>

void run_virtual_hid_devices_state_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "virtual_hid_devices_state"_test = [] {
    {
      krbn::virtual_hid_devices_state virtual_hid_devices_state;

      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

      virtual_hid_devices_state.set_virtual_hid_keyboard_ready(true);
      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == true);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

      virtual_hid_devices_state.set_virtual_hid_keyboard_ready(false);
      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

      virtual_hid_devices_state.set_virtual_hid_pointing_ready(true);
      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == true);

      virtual_hid_devices_state.set_virtual_hid_pointing_ready(false);
      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

      virtual_hid_devices_state.set_virtual_hid_keyboard_ready(true);

      auto json = nlohmann::json::object({
          {"virtual_hid_keyboard_ready", true},
          {"virtual_hid_pointing_ready", false},
      });

      expect(nlohmann::json(virtual_hid_devices_state) == json);
    }
    {
      auto json = nlohmann::json::object({
          {"virtual_hid_keyboard_ready", false},
          {"virtual_hid_pointing_ready", true},
      });

      auto virtual_hid_devices_state = json.get<krbn::virtual_hid_devices_state>();

      expect(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
      expect(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == true);
    }
  };
}
