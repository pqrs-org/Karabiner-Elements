all:
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/SettingsWindow/src/SettingsJSONDiff.swift \
		test.swift -o build/settings_json_diff_test
	build/settings_json_diff_test

clean:
	rm -rf build
