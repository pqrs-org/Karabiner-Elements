all:
	bash build-sparkle.sh

clean:
	cd Sparkle && git clean -d -f -x

update_cget:
	rm -rf cget
	cget install pqrs-org/cget-recipes
	cget install -f cget-requirements.txt
