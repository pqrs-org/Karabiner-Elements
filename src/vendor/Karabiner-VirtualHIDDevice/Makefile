all: gitclean
	make -C src
	mkdir -p dist
	rm -rf dist/org.pqrs.driver.VirtualHIDManager.kext
	cp -R src/build/Release/VirtualHIDManager.kext dist/org.pqrs.driver.VirtualHIDManager.kext
	bash ./scripts/codesign.sh dist
	bash ./scripts/setpermissions.sh dist

install:
	bash ./src/scripts/unload.sh
	sudo rm -rf /tmp/org.pqrs.driver.VirtualHIDDevice
	sudo mkdir -p /tmp/org.pqrs.driver.VirtualHIDDevice
	sudo cp -R dist/org.pqrs.driver.VirtualHIDManager.kext /tmp/org.pqrs.driver.VirtualHIDDevice
	sudo kextload /tmp/org.pqrs.driver.VirtualHIDDevice/org.pqrs.driver.VirtualHIDManager.kext

clean:
	make -C src clean

gitclean:
	git clean -f -x -d
