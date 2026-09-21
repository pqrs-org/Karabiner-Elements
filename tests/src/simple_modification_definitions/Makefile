all:
	$(MAKE) -C ../../../src/apps/localization
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		../../../src/apps/SettingsWindow/src/CanonicalJSON.swift \
		../../../src/apps/SettingsWindow/src/SimpleModificationDefinitions.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionEntry.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionCategory.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionCategories.swift \
		test.swift -o build/simple_modification_definitions_test
	build/simple_modification_definitions_test

clean:
	rm -rf build
