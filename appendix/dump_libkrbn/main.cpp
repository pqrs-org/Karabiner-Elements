#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
void version_updated_callback(void) {
  std::cout << __func__ << std::endl;

  char buffer[1024];
  if (libkrbn_get_version(buffer, sizeof(buffer))) {
    std::cout << "version: " << buffer << std::endl;
  }
}

void manipulator_environment_json_file_updated_callback(void) {
  std::cout << __func__ << std::endl;
}

void frontmost_application_changed_callback(void) {
  std::cout << __func__ << std::endl;

  char bundle_identifier_buffer[1024];
  char file_path_buffer[1024];
  if (libkrbn_get_frontmost_application(bundle_identifier_buffer,
                                        sizeof(bundle_identifier_buffer),
                                        file_path_buffer,
                                        sizeof(file_path_buffer))) {
    std::cout << "  bundle_identifier: " << bundle_identifier_buffer << std::endl;
    std::cout << "  file_path: " << file_path_buffer << std::endl;
  }
}

void connected_devices_updated_callback(void) {
  std::cout << __func__ << std::endl;
}

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  char buffer[32 * 1024];

  libkrbn_initialize();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_version_monitor();
  libkrbn_register_version_updated_callback(version_updated_callback);

  libkrbn_enable_connected_devices_monitor();
  libkrbn_register_connected_devices_updated_callback(connected_devices_updated_callback);
  connected_devices_updated_callback();

  libkrbn_enable_frontmost_application_monitor();
  libkrbn_register_frontmost_application_changed_callback(frontmost_application_changed_callback);
  frontmost_application_changed_callback();

  libkrbn_enable_file_monitors();
  libkrbn_get_manipulator_environment_json_file_path(buffer, sizeof(buffer));
  libkrbn_register_file_updated_callback(buffer, manipulator_environment_json_file_updated_callback);

  {
    libkrbn_enable_complex_modifications_assets_manager();

    libkrbn_complex_modifications_assets_manager_reload();

    {
      auto size = libkrbn_complex_modifications_assets_manager_get_files_size();
      std::cout << "libkrbn_complex_modifications_assets_manager_get_files_size: " << size << std::endl;

      for (size_t i = 0; i < size; ++i) {
        if (libkrbn_complex_modifications_assets_manager_get_file_title(i, buffer, sizeof(buffer))) {
          std::cout << "  " << buffer << std::endl;
        }

        auto rules_size = libkrbn_complex_modifications_assets_manager_get_rules_size(i);
        std::cout << "    libkrbn_complex_modifications_assets_manager_get_rules_size: " << rules_size << std::endl;

        for (size_t j = 0; j < rules_size; ++j) {
          if (libkrbn_complex_modifications_assets_manager_get_rule_description(i, j, buffer, sizeof(buffer))) {
            std::cout << "      " << buffer << std::endl;
          }

          if (j >= 2) {
            if (rules_size - j > 1) {
              std::cout << "      ..." << std::endl;
            }
            break;
          }
        }

        if (i >= 2) {
          if (size - i > 1) {
            std::cout << "  ..." << std::endl;
          }
          break;
        }
      }
    }

    libkrbn_disable_complex_modifications_assets_manager();
  }

  std::cout << std::endl;
  for (int i = 0; i < 10; ++i) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << std::endl;

  std::thread thread([] {
    global_wait->wait_notice();

    libkrbn_terminate();

    CFRunLoopStop(CFRunLoopGetMain());
  });

  // ============================================================

  CFRunLoopRun();

  // ============================================================

  thread.join();

  std::cout << "finished" << std::endl;

  return 0;
}
