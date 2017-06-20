#include "../../src/lib/libkrbn/libkrbn.h"
#include "thread_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  {
    libkrbn_complex_modifications_assets_manager* manager = nullptr;
    if (libkrbn_complex_modifications_assets_manager_initialize(&manager)) {
      libkrbn_complex_modifications_assets_manager_reload(manager);

      {
        auto size = libkrbn_complex_modifications_assets_manager_get_files_size(manager);
        std::cout << "libkrbn_complex_modifications_assets_manager_get_files_size: " << size << std::endl;

        for (size_t i = 0; i < size; ++i) {
          std::cout << "  " << libkrbn_complex_modifications_assets_manager_get_file_title(manager, i) << std::endl;

          auto rules_size = libkrbn_complex_modifications_assets_manager_get_file_rules_size(manager, i);
          std::cout << "    libkrbn_complex_modifications_assets_manager_get_file_rules_size: " << rules_size << std::endl;

          for (size_t j = 0; j < rules_size; ++j) {
            std::cout << "      " << libkrbn_complex_modifications_assets_manager_get_file_rule_description(manager, i, j) << std::endl;
          }
        }
      }

      libkrbn_complex_modifications_assets_manager_terminate(&manager);
    }
  }

  std::cout << std::endl;

  return 0;
}
