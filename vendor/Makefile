all:
	rm -fr vendor
	git -C cpm-cmake-package-lock pull
	cmake -S . -B build
	cmake --build build

clean:
	rm -fr build vendor
	$(MAKE) -C duktape-src clean

.PHONY: build
build:
	$(MAKE) -C duktape-src
