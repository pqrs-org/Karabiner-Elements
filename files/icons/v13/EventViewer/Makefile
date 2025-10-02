all:
	bash ../../scripts/create-icns.sh exported/EventViewer.png
	mv app.icns exported/EventViewer.icns
# Copy files
	cp                 exported/EventViewer.icns            ../../../../src/apps/share/Resources/icons/002-EventViewer.icns
	sips -z 128 128    exported/EventViewer.png    --out    ../../../../src/apps/share/Resources/icons/002-EventViewer.png
	bash ../../scripts/optimize-image.sh                    ../../../../src/apps/share/Resources/icons/002-EventViewer.png
