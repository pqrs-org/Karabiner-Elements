INSTALL_DIR ?= /Library/Application Support/org.pqrs/Karabiner-Elements

.PHONY: all check install format
all: check

check:
	/usr/bin/python3 scripts/localizations.py

install: format
	sudo /usr/bin/install -d "$(INSTALL_DIR)"
	sudo /usr/bin/install -m 644 -o root -g wheel Resources/localizations.json "$(INSTALL_DIR)/localizations.json"

format:
	/usr/bin/python3 scripts/localizations.py --format
