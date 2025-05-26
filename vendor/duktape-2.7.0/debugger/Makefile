NODE:=$(shell { command -v nodejs || command -v node; } 2>/dev/null)
DUKLUV:=$(shell command -v dukluv 2>/dev/null)

# Try to get a useful default --source-dirs which works both in the Duktape
# repo and in the distributable.  We don't want to add '..' because it would
# scan a lot of undesired files in the Duktape repo (e.g. test262 testcases).
ifeq ($(wildcard ../tests/ecmascript/*.js),)
SOURCEDIRS:=../
else
SOURCEDIRS:=../tests/ecmascript
endif

.PHONY: all
all: run

.PHONY: run
run: node_modules static/socket.io-1.2.0.js static/jquery-1.11.1.min.js static/reset.css static/jquery-ui.min.js static/jquery-ui.min.css static/images
	"$(NODE)" duk_debug.js --source-dirs=$(SOURCEDIRS)

rundebug: node_modules static/socket.io-1.2.0.js static/jquery-1.11.1.min.js static/reset.css static/jquery-ui.min.js static/jquery-ui.min.css static/images
	"$(NODE)" duk_debug.js --source-dirs=$(SOURCEDIRS) --verbose --log-messages /tmp/dukdebug-messages --dump-debug-pretty /tmp/dukdebug-pretty

.PHONY: runproxynodejs
runproxynodejs: node_modules static/socket.io-1.2.0.js static/jquery-1.11.1.min.js static/reset.css static/jquery-ui.min.js static/jquery-ui.min.css static/images
	@echo "Running Node.js based debug proxy"
	"$(NODE)" duk_debug.js --json-proxy

.PHONY: runproxydukluv
runproxydukluv: duk_debug_meta.json
	@echo "Running Dukluv based debug proxy (you may need to edit DUKLUV in the Makefile)"
	"$(DUKLUV)" duk_debug_proxy.js --log-level 2 --metadata duk_debug_meta.json --readable-numbers

.PHONY: runproxy
runproxy: runproxydukluv

.PHONY: clean
clean:
	@rm -f static/socket.io-1.2.0.js
	@rm -f static/jquery-1.11.1.min.js
	@rm -f static/jquery.syntaxhighlighter.min.js
	@rm -f static/jquery.snippet.min.js
	@rm -f static/jquery.snippet.min.css
	@rm -f static/prefixfree.min.js
	@rm -f static/reset.css
	@rm -f static/jquery-ui.min.js
	@rm -f static/jquery-ui.min.css
	@rm -rf static/images
	@rm -f jquery-ui-1.11.2.zip
	@rm -rf jquery-ui-1.11.2
	@rm -rf node_modules
	@rm -f duk_debug_meta.json

node_modules:
	npm install

duk_debug_meta.json:
	python2 ../tools/merge_debug_meta.py --output $@ \
		--class-names duk_classnames.yaml \
		--opcodes duk_opcodes.yaml \
		--debug-commands duk_debugcommands.yaml \
		--debug-errors duk_debugerrors.yaml

static/socket.io-1.2.0.js:
	wget -O $@ http://cdn.socket.io/socket.io-1.2.0.js

static/jquery-1.11.1.min.js:
	wget -O $@ http://code.jquery.com/jquery-1.11.1.min.js

# http://balupton.github.io/jquery-syntaxhighlighter/demo/
static/jquery.syntaxhighlighter.min.js:
	wget -O $@ http://balupton.github.com/jquery-syntaxhighlighter/scripts/jquery.syntaxhighlighter.min.js

# http://steamdev.com/snippet/
static/jquery.snippet.min.js:
	wget -O $@ http://steamdev.com/snippet/js/jquery.snippet.min.js
static/jquery.snippet.min.css:
	wget -O $@ http://steamdev.com/snippet/css/jquery.snippet.min.css

# http://prismjs.com/
# http://prismjs.com/plugins/line-highlight/
#
# XXX: prism download manually?

# https://raw.github.com/LeaVerou/prefixfree/gh-pages/prefixfree.min.js
static/prefixfree.min.js:
	wget -O $@ https://raw.github.com/LeaVerou/prefixfree/gh-pages/prefixfree.min.js

# http://meyerweb.com/eric/tools/css/reset/
static/reset.css:
	wget -O $@ http://meyerweb.com/eric/tools/css/reset/reset.css

jquery-ui-1.11.2.zip:
	wget -O $@ http://jqueryui.com/resources/download/jquery-ui-1.11.2.zip
jquery-ui-1.11.2: jquery-ui-1.11.2.zip
	unzip $<
static/jquery-ui.min.js: jquery-ui-1.11.2
	cp jquery-ui-1.11.2/jquery-ui.min.js $@
static/jquery-ui.min.css: jquery-ui-1.11.2
	cp jquery-ui-1.11.2/jquery-ui.min.css $@
static/images: jquery-ui-1.11.2
	cp -r jquery-ui-1.11.2/images static/
