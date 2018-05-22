AUTOUPDATE_ICON = 'build_xcode/build/Release/Karabiner-Elements.app/Contents/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/Resources/AppIcon.icns'

all:
	make -C ../lib/KarabinerKit
	make build_xcode
	install -m 644 ../share/Resources/app.icns $(AUTOUPDATE_ICON)

clean: clean_builds

run:
	open "./build_xcode/build/Release/Karabiner-Elements.app"

include ../../Makefile.rules
