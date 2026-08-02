#include "service_manager.hpp"
#include "services_utility.hpp"

bool krbn_agent_running(const char* service_name) {
  return krbn::services_utility::agent_running(service_name);
}
