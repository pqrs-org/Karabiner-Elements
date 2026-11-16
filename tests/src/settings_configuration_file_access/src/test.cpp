#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_file_access.hpp"
#include <boost/ut.hpp>
#include <fstream>

int main() {
  using namespace boost::ut;
  "configuration file and directory access"_test = [] {
    // root bypasses normal discretionary write permissions.
    expect(::geteuid() != 0) << fatal;

    char pattern[] = "/private/tmp/karabiner-file-access-XXXXXX";
    auto created = ::mkdtemp(pattern);
    expect(created != nullptr);
    if (!created) {
      return;
    }
    auto directory = std::filesystem::path(created);
    auto file = directory / "karabiner.json";

    // First launch must accept missing files and directories that can be created.
    expect(check_configuration_file_access(file) == krbn_configuration_file_access_ok);
    expect(check_configuration_file_access(directory / "new" / "karabiner.json") == krbn_configuration_file_access_ok);
    std::ofstream(file) << "{}";
    expect(check_configuration_file_access(file) == krbn_configuration_file_access_ok);

    // Deny writes to karabiner.json while keeping its parent directory writable.
    ::chmod(file.c_str(), 0400);
    expect(check_configuration_file_access(file) == krbn_configuration_file_access_file_not_writable);

    // Restore file access, then deny creation/replacement in its parent directory.
    ::chmod(file.c_str(), 0600);
    ::chmod(directory.c_str(), 0500);
    expect(check_configuration_file_access(file) == krbn_configuration_file_access_directory_not_writable);

    // Restoring directory access clears the error without changing the file contents.
    ::chmod(directory.c_str(), 0700);
    expect(check_configuration_file_access(file) == krbn_configuration_file_access_ok);
    std::filesystem::remove(file);
    std::filesystem::remove(directory);
  };
}
