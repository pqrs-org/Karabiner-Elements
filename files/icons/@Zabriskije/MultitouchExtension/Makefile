all:
	bash ../../scripts/create-icns.sh exported/MultitouchExtension.png
	mv app.icns exported/MultitouchExtension.icns
# Copy files
	cp                 exported/MultitouchExtension.icns            ../../../../src/apps/share/Resources/icons/001-MultitouchExtension.icns
	sips -z 128 128    exported/MultitouchExtension.png    --out    ../../../../src/apps/share/Resources/icons/001-MultitouchExtension.png
