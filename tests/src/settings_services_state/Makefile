all:
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		-import-objc-header ../../../src/apps/SettingsWindow/src/cpp/settings.hpp \
		../../../src/apps/SettingsWindow/src/SettingsWindowGuidance.swift \
		../../../src/apps/SettingsWindow/src/SettingsWindowAlertState.swift \
		../../../src/apps/SettingsWindow/src/ContentViewStates.swift \
		../../../src/apps/SettingsWindow/src/SettingsConsoleUserServerClient.swift \
		support.swift test.swift -o build/settings_services_state_test
	build/settings_services_state_test

clean:
	rm -rf build
