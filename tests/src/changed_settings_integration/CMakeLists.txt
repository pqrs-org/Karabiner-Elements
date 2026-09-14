cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  libduktape
  "-framework CoreFoundation"
  "-framework IOKit"
  "-framework SystemConfiguration"
)

# Exercise the production ChangedSettings implementation against real C++ snapshots and defaults.
set(settings_swift_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/apps/SettingsWindow/src")
set(changed_settings_executable "${CMAKE_CURRENT_BINARY_DIR}/changed_settings_processor")
set(changed_settings_swift_sources
  "${settings_swift_dir}/SettingsConfiguration.swift"
  "${settings_swift_dir}/SettingsJSONDiff.swift"
  "${settings_swift_dir}/ChangedSettings.swift"
  "${settings_swift_dir}/../../localization/swift/AppLanguage.swift"
  "${settings_swift_dir}/../../localization/swift/AppLocalization.swift"
  "${settings_swift_dir}/CanonicalJSON.swift"
  "${settings_swift_dir}/SimpleModificationDefinitions.swift"
  "${settings_swift_dir}/Models/SimpleModificationDefinitionEntry.swift"
  "${settings_swift_dir}/Models/SimpleModificationDefinitionCategory.swift"
  "${settings_swift_dir}/Models/SimpleModificationDefinitionCategories.swift"
  "${CMAKE_CURRENT_SOURCE_DIR}/process_snapshot.swift"
)
add_custom_command(
  OUTPUT "${changed_settings_executable}"
  COMMAND xcrun swiftc -module-cache-path "${CMAKE_CURRENT_BINARY_DIR}/ModuleCache"
    -swift-version 6 -warnings-as-errors
    ${changed_settings_swift_sources} -o "${changed_settings_executable}"
  DEPENDS ${changed_settings_swift_sources}
  VERBATIM
)
add_custom_target(changed_settings_swift DEPENDS "${changed_settings_executable}")
add_dependencies(karabiner_test changed_settings_swift)
target_compile_definitions(karabiner_test PRIVATE CHANGED_SETTINGS_EXECUTABLE="${changed_settings_executable}")
