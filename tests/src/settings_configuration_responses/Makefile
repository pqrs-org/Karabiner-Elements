all:
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors -parse-as-library \
		../../../src/apps/SettingsWindow/src/ConfigurationSnapshotBuffer.swift \
		../../../src/apps/SettingsWindow/src/ConfigurationLoadingState.swift \
		test.swift loading_state_test.swift -o build/settings_configuration_responses_test
	build/settings_configuration_responses_test

clean:
	rm -rf build
