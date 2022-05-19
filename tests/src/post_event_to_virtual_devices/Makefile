all: build_make
	./build/karabiner_test

clean: clean_builds

overwrite_expected_results:
	./build/overwrite_expected_results

update_input_jsons:
	for f in json/input_event_queue/*.json; do \
		python3 ../../scripts/update-tests-input-json.py $$f > $$f.tmp; \
		mv $$f.tmp $$f; \
		prettier --write $$f; \
	done

include ../Makefile.rules
