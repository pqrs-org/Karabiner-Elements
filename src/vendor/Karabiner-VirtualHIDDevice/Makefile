all: gitclean
	make -C src
	mkdir -p dist
	rm -rf dist/org.pqrs.driver.VirtualHIDManager.kext
	cp -R src/build/Release/VirtualHIDManager.kext dist/org.pqrs.driver.VirtualHIDManager.kext
	bash ./scripts/codesign.sh dist
	bash ./scripts/setpermissions.sh dist

install:
	bash ./src/scripts/unload.sh
	sudo rm -rf /Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext
	bash ./scripts/setpermissions.sh dist
	sudo cp -R dist/org.pqrs.driver.VirtualHIDManager.kext /Library/Extensions
	sudo kextload /Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext

clean:
	make -C src clean

gitclean:
	git clean -f -x -d
