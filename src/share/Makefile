VERSION = `cat ../../version`

all:
	@echo '#pragma once' > karabiner_version.h.tmp
	@echo >> karabiner_version.h.tmp
	@echo 'static const char* karabiner_version = "'$(VERSION)'";' >> karabiner_version.h.tmp
	@cmp --quiet karabiner_version.h.tmp karabiner_version.h || mv karabiner_version.h.tmp karabiner_version.h
	@rm -f karabiner_version.h.tmp
