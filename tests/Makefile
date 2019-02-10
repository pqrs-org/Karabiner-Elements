all:
	@for d in `find lib src -type d | sort`; do \
		if [ -f "$$d/Makefile" ]; then \
			echo "[Test] $$d"; \
			make -C $$d || exit 1; \
		fi; \
	done

clean:
	@for d in `find lib src -type d | sort`; do \
		if [ -f "$$d/Makefile" ]; then \
			make -C $$d clean; \
		fi; \
	done

update_vendor:
	rm -rf vendor
	cget install --prefix vendor pqrs-org/cget-recipes
	cget install --prefix vendor -f cget-requirements.txt
