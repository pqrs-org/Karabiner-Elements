#include "service_manager.hpp"
#include "services_utility.hpp"

bool krbn_daemon_running(const char* service_name) {
  return krbn::services_utility::daemon_running(service_name);
}
