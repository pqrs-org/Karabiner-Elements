all:
	$(MAKE) -C ../../../src/apps/localization
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 test.py
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		test.swift -o build/localization_test
	build/localization_test ../../../src/apps/localization/build/localizations.json

	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/share/swift/Views/OptionKeyObserver.swift \
		option_key_observer_test.swift -o build/option_key_observer_test
	build/option_key_observer_test

	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors \
		../../../src/apps/localization/swift/AppLanguage.swift \
		../../../src/apps/localization/swift/AppLocalization.swift \
		../../../src/apps/localization/swift/AppLocalizedText.swift \
		localized_text_test.swift -o build/localized_text_test
	build/localized_text_test

clean:
	rm -rf build
