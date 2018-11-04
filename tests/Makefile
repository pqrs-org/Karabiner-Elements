all:
	@for d in `find * -type d`; do \
		if [ -f "$$d/Makefile" ]; then \
			echo "[Test] $$d"; \
			make -C $$d || exit 1; \
		fi; \
	done

clean:
	@for d in `find * -type d`; do \
		if [ -f "$$d/Makefile" ]; then \
			make -C $$d clean; \
		fi; \
	done

update_vendor:
	cget install -f cget-requirements.txt --prefix vendor
