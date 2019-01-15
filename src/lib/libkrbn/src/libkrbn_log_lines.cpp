#include "libkrbn/impl/libkrbn_log_monitor.hpp"

size_t libkrbn_log_lines_get_size(libkrbn_log_lines* p) {
  auto log_lines = reinterpret_cast<libkrbn_log_lines_class*>(p);
  if (!log_lines) {
    return 0;
  }

  auto lines = log_lines->get_lines();
  if (!lines) {
    return 0;
  }

  return lines->size();
}

const char* libkrbn_log_lines_get_line(libkrbn_log_lines* p, size_t index) {
  auto log_lines = reinterpret_cast<libkrbn_log_lines_class*>(p);
  if (!log_lines) {
    return nullptr;
  }

  auto lines = log_lines->get_lines();
  if (!lines) {
    return nullptr;
  }

  if (index >= lines->size()) {
    return nullptr;
  }

  return (*lines)[index].c_str();
}

bool libkrbn_log_lines_is_warn_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::warn;
}

bool libkrbn_log_lines_is_error_line(const char* line) {
  return pqrs::spdlog::find_level(line) == spdlog::level::err;
}
