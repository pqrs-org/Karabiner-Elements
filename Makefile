VERSION = `head -n 1 version`

CLANG_FORMAT_FILES = \
	'*.h' \
	'*.hpp' \
	'*.hpp.in' \
	'*.iig' \
	'*.mm' \
	'*.cpp' \
	':(exclude)docs/vendor/**' \
	':(exclude)vendor/**'

all:
	@echo "Read a document at the following URL to build a distributable package."
	@echo "https://github.com/pqrs-org/Karabiner-Elements/#how-to-build"

package: clean
	./make-package.sh
	$(MAKE) clean-launch-services-database

build:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf pkgroot
	rm -f *.dmg

clean-launch-services-database:
	$(MAKE) -C tools/clean-launch-services-database

gitclean:
	git clean -f -x -d
	(cd vendor/cpm-cmake-package-lock && git clean -f -x -d)
	(cd vendor/Karabiner-DriverKit-VirtualHIDDevice && git clean -f -x -d)

notarize:
	xcrun notarytool \
		submit Karabiner-Elements-$(VERSION).dmg \
		--keychain-profile "pqrs.org notarization" \
		--wait
	$(MAKE) staple
	say "notarization completed"

staple:
	xcrun stapler staple Karabiner-Elements-$(VERSION).dmg

format: clang-format swift-format shell-format ruff-format prettier-format test-json-format localization-format

clang-format:
	git ls-files -z -- $(CLANG_FORMAT_FILES) | xargs -0 clang-format -i

swift-format:
	find src/apps tests -name '*.swift' -print0 | xargs -0 swift-format -i

shell-format:
	git ls-files -z -- '*.sh' 'pkginfo/Scripts/*' | xargs -0 shfmt -w -i 4 -ci

ruff-format:
	git ls-files -z -- '*.py' ':(exclude)vendor/**' ':(exclude)docs/vendor/**' | xargs -0 ruff format

prettier-format:
	@echo "Formatting JSON and Markdown with Prettier..."
	@prettier_log=$$(mktemp); \
	trap 'rm -f "$$prettier_log"' EXIT; \
	git ls-files -z -- \
		'*.json' \
		'*.md' \
		':(exclude)vendor/**' \
		':(exclude)docs/vendor/**' \
		':(exclude)**/*.icon/icon.json' \
		':(exclude)**/Assets.xcassets/**' \
		':(exclude)src/apps/localization/Resources/**' \
		':(exclude)tests/src/complex_modifications_assets/json/complex_modifications/broken.json' \
		':(exclude)tests/src/complex_modifications_assets/json/complex_modifications/broken_link.json' \
		':(exclude)tests/src/complex_modifications_assets/json/complex_modifications/link.json' \
		':(exclude)tests/src/complex_modifications_assets/json/lint/assets/broken.json' \
		':(exclude)tests/src/core_configuration/json/broken.json' \
		':(exclude)tests/src/manipulator/json/manipulator_manager/test_cases/**' \
		':(exclude)tests/src/post_event_to_virtual_devices/json/test_cases/**' | \
		xargs -0 prettier --write > "$$prettier_log"; \
	prettier_status=$$?; \
	sed '/ (unchanged)$$/d' "$$prettier_log"; \
	exit $$prettier_status

test-json-format: ruff-format prettier-format
	/usr/bin/python3 tests/scripts/format_test_json.py \
		tests/src/manipulator/json/manipulator_manager/test_cases \
		tests/src/post_event_to_virtual_devices/json/test_cases

localization-format:
	$(MAKE) -C src/apps/localization format

swiftlint:
	swiftlint

notarized-dmg: package notarize
