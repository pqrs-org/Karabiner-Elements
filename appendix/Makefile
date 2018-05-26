all:
	@for d in `find * -type d`; do \
		if [ -f "$$d/Makefile" ]; then \
			echo "$$d"; \
			make -C $$d || exit 1; \
		fi; \
	done

clean:
	@for d in `find * -type d`; do \
		if [ -f "$$d/Makefile" ]; then \
			echo "$$d"; \
			make -C $$d clean || exit 1; \
		fi; \
	done
