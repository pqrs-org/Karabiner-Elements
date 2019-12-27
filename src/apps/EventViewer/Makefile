all:
	make -C ../lib/KarabinerKit
	make build_xcode

clean: clean_builds

run:
	bash ../../../scripts/codesign.sh build_xcode
	# Do not execute Karabiner-EventViewer.app/Contents/MacOS/Karabiner-EventViewer directly.
	# We have to run as app in order to apply the user approval for Input Monitoring.
	open ./build_xcode/build/Release/Karabiner-EventViewer.app

include ../../Makefile.rules
