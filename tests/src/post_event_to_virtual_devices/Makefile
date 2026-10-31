all: build_make
	MallocNanoZone=0 ./build/karabiner_test

clean: clean_builds

overwrite_expected_results:
	./build/overwrite_expected_results
	/usr/bin/python3 ../../scripts/format_test_json.py \
		json/test_cases/*_result.json

update_input_jsons:
	/usr/bin/python3 ../../scripts/format_test_json.py \
		json/test_cases/*_input.jsonc

include ../Makefile.rules
