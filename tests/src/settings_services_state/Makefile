BUILD_DIR ?= build

all:
	mkdir -p "$(BUILD_DIR)"
	xcrun swiftc -module-cache-path "$(BUILD_DIR)/ModuleCache" -swift-version 6 -warnings-as-errors \
		-import-objc-header ../../../src/apps/SettingsWindow/src/cpp/settings.hpp \
		../../../src/apps/SettingsWindow/src/SettingsWindowGuidance.swift \
		../../../src/apps/SettingsWindow/src/SettingsWindowAlertState.swift \
		../../../src/apps/SettingsWindow/src/ContentViewStates.swift \
		../../../src/apps/SettingsWindow/src/SettingsConsoleUserServerClient.swift \
		support.swift test.swift -o "$(BUILD_DIR)/settings_services_state_test"
	"$(BUILD_DIR)/settings_services_state_test"

clean:
	rm -rf "$(BUILD_DIR)"
