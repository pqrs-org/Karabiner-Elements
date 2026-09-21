all:
	$(MAKE) -C ../../../src/apps/localization
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		../../../src/apps/SettingsWindow/src/SettingsJSONDiff.swift \
		../../../src/apps/SettingsWindow/src/ChangedSettings.swift \
		test.swift -o build/changed_settings_test
	build/changed_settings_test ../../../src/apps/localization/build/localizations.json

clean:
	rm -rf build
