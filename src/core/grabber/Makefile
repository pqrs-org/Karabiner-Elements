all:
	mkdir -p build && cd build && cmake .. && make

clean:
	rm -rf build

install: all
	sudo install -m 755 ./build/karabiner_grabber '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_grabber'
	sudo killall karabiner_grabber
