VERSIONSIGNATURE = $(shell scripts/makeversionsignature.sh)
KEXT_DIRECTORY = /Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice/Extensions
KEXT = $(KEXT_DIRECTORY)/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext

all: gitclean
	bash scripts/setversion.sh
	make -C src
	rm -rf dist
	mkdir -p dist
	mkdir -p dist/include
	cp -R src/build/Release/VirtualHIDDevice.kext dist/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext
	cp src/include/karabiner_virtual_hid_device.hpp dist/include
	cp src/include/karabiner_virtual_hid_device_methods.hpp dist/include
	cp scripts/uninstall.sh dist
	bash ./scripts/codesign.sh dist
	bash ./scripts/setpermissions.sh dist

install:
	sudo rm -rf '$(KEXT)'
	sudo mkdir -p '$(KEXT_DIRECTORY)'
	sudo cp -R dist/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext '$(KEXT)'
	sudo kextload '$(KEXT)'

tmpinstall:
	sudo rm -rf /tmp/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext
	sudo cp -R dist/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext /tmp
	sudo kextload /tmp/org.pqrs.driver.Karabiner.VirtualHIDDevice.$(VERSIONSIGNATURE).kext

uninstall:
	sudo sh dist/uninstall.sh

reload:
	make uninstall
	make install

clean:
	make -C src clean

gitclean:
	git clean -f -x -d
