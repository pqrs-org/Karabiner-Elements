#include "../../src/lib/libkrbn/libkrbn.h"
#include "thread_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  {
    libkrbn_complex_modifications_assets_manager* manager;
    if (libkrbn_complex_modifications_assets_manager_initialize(&manager)) {
      libkrbn_complex_modifications_assets_manager_reload(manager);

      std::cout << "libkrbn_complex_modifications_assets_manager_get_files_size: "
                << libkrbn_complex_modifications_assets_manager_get_files_size(manager)
                << std::endl;

      libkrbn_complex_modifications_assets_manager_terminate(&manager);
    }
  }

  std::cout << std::endl;

  return 0;
}
