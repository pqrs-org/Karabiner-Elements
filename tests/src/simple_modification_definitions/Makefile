BUILD_DIR ?= build

all:
	mkdir -p "$(BUILD_DIR)"
	xcrun swiftc -module-cache-path "$(BUILD_DIR)/ModuleCache" -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		../../../src/apps/SettingsWindow/src/CanonicalJSON.swift \
		../../../src/apps/SettingsWindow/src/SimpleModificationDefinitions.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionEntry.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionCategory.swift \
		../../../src/apps/SettingsWindow/src/Models/SimpleModificationDefinitionCategories.swift \
		test.swift -o "$(BUILD_DIR)/simple_modification_definitions_test"
	"$(BUILD_DIR)/simple_modification_definitions_test"

clean:
	rm -rf "$(BUILD_DIR)"
