all: build_make
	MallocNanoZone=0 ./build/karabiner_test

clean: clean_builds

overwrite_expected_results:
	./build/overwrite_expected_results

update_input_jsons:
	python3 ../../scripts/update_tests_input_json.py \
		json/manipulator_manager/input_event_queue/*.json
	prettier --write \
		json/manipulator_manager/input_event_queue/*.json

include ../Makefile.rules
