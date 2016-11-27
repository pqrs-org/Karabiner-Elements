all: gitclean
	bash scripts/setversion.sh
	make -C src
	rm -rf dist
	mkdir -p dist
	mkdir -p dist/include
	cp -R src/build/Release/VirtualHIDDevice.kext dist/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
	cp src/include/karabiner_virtual_hid_device.hpp dist/include
	cp src/include/karabiner_virtual_hid_device_methods.hpp dist/include
	cp scripts/update.sh dist
	cp scripts/uninstall.sh dist
	bash ./scripts/codesign.sh dist
	bash ./scripts/setpermissions.sh dist

install:
	sudo sh dist/update.sh

clean:
	make -C src clean

gitclean:
	git clean -f -x -d
