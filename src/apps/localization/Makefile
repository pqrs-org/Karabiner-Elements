INSTALL_DIR ?= /Library/Application Support/org.pqrs/Karabiner-Elements

.PHONY: all check install format
all: check

check:
	/usr/bin/python3 scripts/localizations.py

install: format
	sudo /usr/bin/install -d "$(INSTALL_DIR)/localizations"
	sudo /usr/bin/rsync -rt --delete --exclude='.*' --chmod=Du=rwx,Dgo=rx,Fu=rw,Fgo=r Resources/ "$(INSTALL_DIR)/localizations/"

format: check
	/usr/bin/python3 scripts/localizations.py --format
