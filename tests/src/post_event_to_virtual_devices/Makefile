all: build_make
	MallocNanoZone=0 ./build/karabiner_test

clean: clean_builds

overwrite_expected_results:
	./build/overwrite_expected_results

update_input_jsons:
	prettier --write \
		json/test_cases/*_input.jsonc

include ../Makefile.rules
