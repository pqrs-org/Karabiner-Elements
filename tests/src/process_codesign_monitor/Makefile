all: build_make
	MallocNanoZone=0 ./build/karabiner_test
	MallocNanoZone=0 ./build/karabiner_integration_test

prepare-fixtures: build_make
	mkdir -p fixtures/process_codesign_test_helper_a.app/Contents/MacOS
	mkdir -p fixtures/process_codesign_test_helper_b.app/Contents/MacOS
	cp src/app/Info.plist fixtures/process_codesign_test_helper_a.app/Contents/Info.plist
	cp src/app/Info.plist fixtures/process_codesign_test_helper_b.app/Contents/Info.plist
	cp build/process_codesign_test_helper_a fixtures/process_codesign_test_helper_a.app/Contents/MacOS/process_codesign_test_helper
	cp build/process_codesign_test_helper_b fixtures/process_codesign_test_helper_b.app/Contents/MacOS/process_codesign_test_helper
	bash ../../../scripts/codesign.sh fixtures
	for f in fixtures/*.app; do codesign -dvv $$f; echo; done

clean: clean_builds

include ../Makefile.rules
