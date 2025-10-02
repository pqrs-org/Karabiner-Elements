all:
	bash ../../scripts/create-icns.sh exported/KarabinerElements.png
	mv app.icns exported/KarabinerElements.icns
# Copy files
	cp                 exported/KarabinerElements.icns            ../../../../src/apps/share/Resources/icons/001-KarabinerElements.icns
	sips -z 128 128    exported/KarabinerElements.png    --out    ../../../../src/apps/share/Resources/icons/001-KarabinerElements.png
	bash ../../scripts/optimize-image.sh                          ../../../../src/apps/share/Resources/icons/001-KarabinerElements.png
