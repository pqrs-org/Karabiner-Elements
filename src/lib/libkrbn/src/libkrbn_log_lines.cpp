#include "libkrbn/impl/libkrbn_components_manager.hpp"
#include "libkrbn/impl/libkrbn_cpp.hpp"
#include "libkrbn/impl/libkrbn_log_monitor.hpp"

namespace {
std::shared_ptr<std::deque<std::string>> get_current_log_lines(void) {
  if (auto manager = libkrbn_cpp::get_components_manager()) {
    return manager->get_current_log_lines();
  }

  return nullptr;
}
} // namespace

size_t libkrbn_log_lines_get_size(void) {
  if (auto lines = get_current_log_lines()) {
    return lines->size();
  }

  return 0;
}

bool libkrbn_log_lines_get_line(size_t index,
                                char* buffer,
                                size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto lines = get_current_log_lines()) {
    if (index < lines->size()) {
      strlcpy(buffer, (*lines)[index].c_str(), length);
      return true;
    }
  }

  return false;
}

bool libkrbn_log_lines_is_warn_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::warn;
}

bool libkrbn_log_lines_is_error_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::err;
}

uint64_t libkrbn_log_lines_get_date_number(const char* line) {
  if (auto n = pqrs::spdlog::find_date_number(line)) {
    return *n;
  }
  return 0;
}
