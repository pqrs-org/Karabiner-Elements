#include "app_icon_utility.hpp"
#include "app_icon.hpp"
#include "constants.hpp"

int krbn_get_app_icon_number(void) {
  auto icon = krbn::app_icon(krbn::constants::get_system_app_icon_configuration_file_path());
  return icon.get_number();
}
