INSTALL_DIR ?= /Library/Application Support/org.pqrs/Karabiner-Elements

.PHONY: all check install format clean
all:
	/usr/bin/python3 scripts/localizations.py --output build/localizations.json

check:
	/usr/bin/python3 scripts/localizations.py

install: format
	$(MAKE) all
	sudo /usr/bin/install -m 644 build/localizations.json "$(INSTALL_DIR)/localizations.json"

format: check
	/usr/bin/python3 scripts/localizations.py --format

clean:
	rm -rf build
