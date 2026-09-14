BUILD_DIR ?= build

all:
	mkdir -p "$(BUILD_DIR)"
	xcrun swiftc -module-cache-path "$(BUILD_DIR)/ModuleCache" -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		../../../src/apps/SettingsWindow/src/SettingsJSONDiff.swift \
		../../../src/apps/SettingsWindow/src/ChangedSettings.swift \
		test.swift -o "$(BUILD_DIR)/changed_settings_test"
	"$(BUILD_DIR)/changed_settings_test" ../../../src/apps/localization/Resources

clean:
	rm -rf "$(BUILD_DIR)"
