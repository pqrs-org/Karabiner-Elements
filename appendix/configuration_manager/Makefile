CONFIGURATION = Release

all:
	xcodebuild -alltargets -configuration $(CONFIGURATION) build

clean:
	rm -rf build
	rm -f *.xcodeproj/*.mode1
	rm -f *.xcodeproj/*.mode1v3
	rm -f *.xcodeproj/*.pbxuser
	rm -rf *.xcodeproj/xcuserdata
	rm -rf *.xcodeproj/project.xcworkspace/xcuserdata
	rm -f *.app.zip

xcode:
	open *.xcodeproj

run:
	./build/Release/configuration_manager

test:
	rm -rf dot_karabiner_directory/configuration
	rm -rf dot_karabiner_directory/configuration.back
	rm -rf dot_karabiner_directory/file-synchronisation-service
	sleep 0.5
	mkdir dot_karabiner_directory/configuration
	sleep 0.5
	date > dot_karabiner_directory/configuration/1.json
	sleep 0.5
	date > dot_karabiner_directory/configuration/2.json
	sleep 0.5
	date > dot_karabiner_directory/configuration/1.json
	sleep 0.5
	mv dot_karabiner_directory/configuration dot_karabiner_directory/configuration.back
	sleep 0.5
	mkdir dot_karabiner_directory/file-synchronisation-service
	sleep 0.5
	ln -s file-synchronisation-service dot_karabiner_directory/configuration
	sleep 0.5
	date > dot_karabiner_directory/configuration/3.json
	sleep 0.5
	date > dot_karabiner_directory/configuration/4.json
	sleep 0.5
	date > dot_karabiner_directory/configuration/3.json
