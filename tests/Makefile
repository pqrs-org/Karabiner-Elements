all:
	@for d in `find * -type d`; do if [ -f "$$d/Makefile" ]; then echo "[Test] $$d"; make -C $$d run || exit 1; fi; done

clean:
	@for d in `find * -type d`; do if [ -f "$$d/Makefile" ]; then make -C $$d clean; fi; done
