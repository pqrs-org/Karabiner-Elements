BUILD_DIR ?= build

all:
	mkdir -p "$(BUILD_DIR)"
	xcrun swiftc -module-cache-path "$(BUILD_DIR)/ModuleCache" -swift-version 6 -warnings-as-errors \
		../../../src/apps/SettingsWindow/src/SettingsJSONDiff.swift \
		test.swift -o "$(BUILD_DIR)/settings_json_diff_test"
	"$(BUILD_DIR)/settings_json_diff_test"

clean:
	rm -rf "$(BUILD_DIR)"
