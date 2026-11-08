#include "elecom/trackball_test.hpp"
#include "logitech/hidpp/button_test.hpp"

int main() {
  run_elecom_trackball_test();
  run_logitech_hidpp_button_test();
  return 0;
}
