all: clean
	./make-package.sh

build:
	$(MAKE) -C pkginfo
	$(MAKE) -C src

clean:
	$(MAKE) -C pkginfo clean
	$(MAKE) -C src clean
	rm -rf pkgroot
	rm -f *.dmg

gitclean:
	git clean -f -x -d

ibtool-upgrade:
	find * -name '*.xib' | while read f; do xcrun ibtool --upgrade "$$f"; done
