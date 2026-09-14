BUILD_DIR ?= build

all:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 test.py
	mkdir -p "$(BUILD_DIR)"
	xcrun swiftc -module-cache-path "$(BUILD_DIR)/ModuleCache" -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		test.swift -o "$(BUILD_DIR)/localization_test"
	"$(BUILD_DIR)/localization_test" ../../../src/apps/localization/Resources

clean:
	rm -rf "$(BUILD_DIR)"
