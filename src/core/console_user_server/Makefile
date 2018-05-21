all:
	mkdir -p build && cd build && cmake .. && make

clean:
	rm -rf build

install: all
	sudo install -m 755 ./build/karabiner_console_user_server '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_console_user_server'
	sudo killall karabiner_console_user_server
