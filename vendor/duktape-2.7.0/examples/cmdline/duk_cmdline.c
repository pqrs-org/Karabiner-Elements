/*
 *  Command line execution tool.  Useful for test cases and manual testing.
 *  Also demonstrates some basic integration techniques.
 *
 *  Optional features include:
 *
 *  - To enable print()/alert() bindings, define DUK_CMDLINE_PRINTALERT_SUPPORT
 *    and add extras/print-alert/duk_print_alert.c to compilation.
 *
 *  - To enable console.log() etc, define DUK_CMDLINE_CONSOLE_SUPPORT
 *    and add extras/console/duk_console.c to compilation.
 *
 *  - To enable Duktape.Logger, define DUK_CMDLINE_LOGGING_SUPPORT
 *    and add extras/logging/duk_logging.c to compilation.
 *
 *  - To enable Duktape 1.x module loading support (require(),
 *    Duktape.modSearch() etc), define DUK_CMDLINE_MODULE_SUPPORT and add
 *    extras/module-duktape/duk_module_duktape.c to compilation.
 *
 *  - To enable linenoise and other fancy stuff, compile with -DDUK_CMDLINE_FANCY.
 *    It is not the default to maximize portability.  You can also compile in
 *    support for example allocators, grep for DUK_CMDLINE_*.
 */

/* Helper define to enable a feature set; can also use separate defines. */
#if defined(DUK_CMDLINE_FANCY)
#define DUK_CMDLINE_LINENOISE
#define DUK_CMDLINE_LINENOISE_COMPLETION  /* Enables completion and hints. */
#define DUK_CMDLINE_RLIMIT
#define DUK_CMDLINE_SIGNAL
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
    defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
/* Suppress warnings about plain fopen() etc. */
#define _CRT_SECURE_NO_WARNINGS
#if defined(_MSC_VER) && (_MSC_VER < 1900)
/* Workaround for snprintf() missing in older MSVC versions.
 * Note that _snprintf() may not NUL terminate the string, but
 * this difference does not matter here as a NUL terminator is
 * always explicitly added.
 */
#define snprintf _snprintf
#endif
#endif

#if defined(DUK_CMDLINE_PTHREAD_STACK_CHECK)
#define _GNU_SOURCE
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(DUK_CMDLINE_SIGNAL)
#include <signal.h>
#endif
#if defined(DUK_CMDLINE_RLIMIT)
#include <sys/resource.h>
#endif
#if defined(DUK_CMDLINE_LINENOISE)
#include "linenoise.h"
#include <stdint.h>  /* Assume C99/C++11 with linenoise. */
#endif
#if defined(DUK_CMDLINE_PRINTALERT_SUPPORT)
#include "duk_print_alert.h"
#endif
#if defined(DUK_CMDLINE_CONSOLE_SUPPORT)
#include "duk_console.h"
#endif
#if defined(DUK_CMDLINE_LOGGING_SUPPORT)
#include "duk_logging.h"
#endif
#if defined(DUK_CMDLINE_MODULE_SUPPORT)
#include "duk_module_duktape.h"
#endif
#if defined(DUK_CMDLINE_FILEIO)
#include <errno.h>
#endif
#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif
#if defined(DUK_CMDLINE_ALLOC_LOGGING)
#include "duk_alloc_logging.h"
#endif
#if defined(DUK_CMDLINE_ALLOC_TORTURE)
#include "duk_alloc_torture.h"
#endif
#if defined(DUK_CMDLINE_ALLOC_HYBRID)
#include "duk_alloc_hybrid.h"
#endif
#include "duktape.h"

#include "duk_cmdline.h"

#if defined(DUK_CMDLINE_LOWMEM)
#include "duk_alloc_pool.h"
#endif

#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
#include "duk_trans_socket.h"
#endif

#define  MEM_LIMIT_NORMAL   (128*1024*1024)   /* 128 MB */
#define  MEM_LIMIT_HIGH     (2047*1024*1024)  /* ~2 GB */

static int main_argc = 0;
static char **main_argv = NULL;
static int interactive_mode = 0;
static int allow_bytecode = 0;
#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
static int debugger_reattach = 0;
#endif
#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
static int no_auto_complete = 0;
#endif

int duk_cmdline_stack_check(void);

/*
 *  Misc helpers
 */

static void print_greet_line(void) {
	printf("((o) Duktape%s %d.%d.%d (%s)\n",
#if defined(DUK_CMDLINE_LINENOISE)
	       " [linenoise]",
#else
	       "",
#endif
	       (int) (DUK_VERSION / 10000),
	       (int) ((DUK_VERSION / 100) % 100),
	       (int) (DUK_VERSION % 100),
	       DUK_GIT_DESCRIBE);
}

#if defined(DUK_CMDLINE_RLIMIT)
static void set_resource_limits(rlim_t mem_limit_value) {
	int rc;
	struct rlimit lim;

	rc = getrlimit(RLIMIT_AS, &lim);
	if (rc != 0) {
		fprintf(stderr, "Warning: cannot read RLIMIT_AS\n");
		return;
	}

	if (lim.rlim_max < mem_limit_value) {
		fprintf(stderr, "Warning: rlim_max < mem_limit_value (%d < %d)\n", (int) lim.rlim_max, (int) mem_limit_value);
		return;
	}

	lim.rlim_cur = mem_limit_value;
	lim.rlim_max = mem_limit_value;

	rc = setrlimit(RLIMIT_AS, &lim);
	if (rc != 0) {
		fprintf(stderr, "Warning: setrlimit failed\n");
		return;
	}

#if 0
	fprintf(stderr, "Set RLIMIT_AS to %d\n", (int) mem_limit_value);
#endif
}
#endif  /* DUK_CMDLINE_RLIMIT */

#if defined(DUK_CMDLINE_SIGNAL)
static void my_sighandler(int x) {
	fprintf(stderr, "Got signal %d\n", x);
	fflush(stderr);
}
static void set_sigint_handler(void) {
	(void) signal(SIGINT, my_sighandler);
	(void) signal(SIGPIPE, SIG_IGN);  /* avoid SIGPIPE killing process */
}
#endif  /* DUK_CMDLINE_SIGNAL */

static void cmdline_fatal_handler(void *udata, const char *msg) {
	(void) udata;
	fprintf(stderr, "*** FATAL ERROR: %s\n", msg ? msg : "no message");
	fprintf(stderr, "Causing intentional segfault...\n");
	fflush(stderr);
	*((volatile unsigned int *) 0) = (unsigned int) 0xdeadbeefUL;
	abort();
}

/* Print error to stderr and pop error. */
static void print_pop_error(duk_context *ctx, FILE *f) {
	fprintf(f, "%s\n", duk_safe_to_stacktrace(ctx, -1));
	fflush(f);
	duk_pop(ctx);
}

static duk_ret_t wrapped_compile_execute(duk_context *ctx, void *udata) {
	const char *src_data;
	duk_size_t src_len;
	duk_uint_t comp_flags;

	(void) udata;

	/* XXX: Here it'd be nice to get some stats for the compilation result
	 * when a suitable command line is given (e.g. code size, constant
	 * count, function count.  These are available internally but not through
	 * the public API.
	 */

	/* Use duk_compile_lstring_filename() variant which avoids interning
	 * the source code.  This only really matters for low memory environments.
	 */

	/* [ ... bytecode_filename src_data src_len filename ] */

	src_data = (const char *) duk_require_pointer(ctx, -3);
	src_len = (duk_size_t) duk_require_uint(ctx, -2);

	if (src_data != NULL && src_len >= 1 && src_data[0] == (char) 0xbf) {
		/* Bytecode. */
		if (allow_bytecode) {
			void *buf;
			buf = duk_push_fixed_buffer(ctx, src_len);
			memcpy(buf, (const void *) src_data, src_len);
			duk_load_function(ctx);
		} else {
			(void) duk_type_error(ctx, "bytecode input rejected (use -b to allow bytecode inputs)");
		}
	} else {
		/* Source code. */
		comp_flags = DUK_COMPILE_SHEBANG;
		duk_compile_lstring_filename(ctx, comp_flags, src_data, src_len);
	}

	/* [ ... bytecode_filename src_data src_len function ] */

	/* Optional bytecode dump. */
	if (duk_is_string(ctx, -4)) {
		FILE *f;
		void *bc_ptr;
		duk_size_t bc_len;
		size_t wrote;
		char fnbuf[256];
		const char *filename;

		duk_dup_top(ctx);
		duk_dump_function(ctx);
		bc_ptr = duk_require_buffer_data(ctx, -1, &bc_len);
		filename = duk_require_string(ctx, -5);
#if defined(EMSCRIPTEN)
		if (filename[0] == '/') {
			snprintf(fnbuf, sizeof(fnbuf), "%s", filename);
		} else {
			snprintf(fnbuf, sizeof(fnbuf), "/working/%s", filename);
		}
#else
		snprintf(fnbuf, sizeof(fnbuf), "%s", filename);
#endif
		fnbuf[sizeof(fnbuf) - 1] = (char) 0;

		f = fopen(fnbuf, "wb");
		if (!f) {
			(void) duk_generic_error(ctx, "failed to open bytecode output file");
		}
		wrote = fwrite(bc_ptr, 1, (size_t) bc_len, f);  /* XXX: handle partial writes */
		(void) fclose(f);
		if (wrote != bc_len) {
			(void) duk_generic_error(ctx, "failed to write all bytecode");
		}

		return 0;  /* duk_safe_call() cleans up */
	}

#if 0
	/* Manual test for bytecode dump/load cycle: dump and load before
	 * execution.  Enable manually, then run "make ecmatest" for a
	 * reasonably good coverage of different functions and programs.
	 */
	duk_dump_function(ctx);
	duk_load_function(ctx);
#endif

#if defined(DUK_CMDLINE_LOWMEM)
	lowmem_start_exec_timeout();
#endif

	duk_push_global_object(ctx);  /* 'this' binding */
	duk_call_method(ctx, 0);

#if defined(DUK_CMDLINE_LOWMEM)
	lowmem_clear_exec_timeout();
#endif

	if (interactive_mode) {
		/*
		 *  In interactive mode, write to stdout so output won't
		 *  interleave as easily.
		 *
		 *  NOTE: the ToString() coercion may fail in some cases;
		 *  for instance, if you evaluate:
		 *
		 *    ( {valueOf: function() {return {}},
		 *       toString: function() {return {}}});
		 *
		 *  The error is:
		 *
		 *    TypeError: coercion to primitive failed
		 *            duk_api.c:1420
		 *
		 *  These are handled now by the caller which also has stack
		 *  trace printing support.  User code can print out errors
		 *  safely using duk_safe_to_string().
		 */

		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "dukFormat");
		duk_dup(ctx, -3);
		duk_call(ctx, 1);  /* -> [ ... res stash formatted ] */

		fprintf(stdout, "= %s\n", duk_to_string(ctx, -1));
		fflush(stdout);
	} else {
		/* In non-interactive mode, success results are not written at all.
		 * It is important that the result value is not string coerced,
		 * as the string coercion may cause an error in some cases.
		 */
	}

	return 0;  /* duk_safe_call() cleans up */
}

/*
 *  Minimal Linenoise completion support
 */

#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
static duk_context *completion_ctx;

static const char *linenoise_completion_script =
	"(function linenoiseCompletion(input, addCompletion) {\n"
	"    // Find maximal trailing string which looks like a property\n"
	"    // access.  Look up all the components (starting from the global\n"
	"    // object now) except the last; treat the last component as a\n"
	"    // partial name and use it as a filter for possible properties.\n"
	"    var match, propseq, obj, i, partial, names, name, sanity;\n"
	"\n"
	"    if (!input) { return; }\n"
	"    match = /^.*?((?:\\w+\\.)*\\w*)$/.exec(input);\n"
	"    if (!match || !match[1]) { return; }\n"
	"    var propseq = match[1].split('.');\n"
	"\n"
	"    obj = Function('return this')();\n"
	"    for (i = 0; i < propseq.length - 1; i++) {\n"
	"        if (obj === void 0 || obj === null) { return; }\n"
	"        obj = obj[propseq[i]];\n"
	"    }\n"
	"    if (obj === void 0 || obj === null) { return; }\n"
	"\n"
	"    partial = propseq[propseq.length - 1];\n"
	"    sanity = 1000;\n"
	"    while (obj != null) {\n"
	"        if (--sanity < 0) { throw new Error('sanity'); }\n"
	"        names = Object.getOwnPropertyNames(Object(obj));\n"
	"        for (i = 0; i < names.length; i++) {\n"
	"            if (--sanity < 0) { throw new Error('sanity'); }\n"
	"            name = names[i];\n"
	"            if (Number(name) >= 0) { continue; }  // ignore array keys\n"
	"            if (name.substring(0, partial.length) !== partial) { continue; }\n"
	"            if (name === partial) { addCompletion(input + '.'); continue; }\n"
	"            addCompletion(input + name.substring(partial.length));\n"
	"        }\n"
	"        obj = Object.getPrototypeOf(Object(obj));\n"
	"    }\n"
	"})";

static const char *linenoise_hints_script =
	"(function linenoiseHints(input) {\n"
	"    // Similar to completions but different handling for final results.\n"
	"    var match, propseq, obj, i, partial, names, name, res, found, first, sanity;\n"
	"\n"
	"    if (!input) { return; }\n"
	"    match = /^.*?((?:\\w+\\.)*\\w*)$/.exec(input);\n"
	"    if (!match || !match[1]) { return; }\n"
	"    var propseq = match[1].split('.');\n"
	"\n"
	"    obj = Function('return this')();\n"
	"    for (i = 0; i < propseq.length - 1; i++) {\n"
	"        if (obj === void 0 || obj === null) { return; }\n"
	"        obj = obj[propseq[i]];\n"
	"    }\n"
	"    if (obj === void 0 || obj === null) { return; }\n"
	"\n"
	"    partial = propseq[propseq.length - 1];\n"
	"    res = [];\n"
	"    found = Object.create(null);  // keys already handled\n"
	"    sanity = 1000;\n"
	"    while (obj != null) {\n"
	"        if (--sanity < 0) { throw new Error('sanity'); }\n"
	"        names = Object.getOwnPropertyNames(Object(obj));\n"
	"        first = true;\n"
	"        for (i = 0; i < names.length; i++) {\n"
	"            if (--sanity < 0) { throw new Error('sanity'); }\n"
	"            name = names[i];\n"
	"            if (Number(name) >= 0) { continue; }  // ignore array keys\n"
	"            if (name.substring(0, partial.length) !== partial) { continue; }\n"
	"            if (name === partial) { continue; }\n"
	"            if (found[name]) { continue; }\n"
	"            found[name] = true;\n"
	"            res.push(res.length === 0 ? name.substring(partial.length) : (first ? ' || ' : ' | ') + name);\n"
	"            first = false;\n"
	"        }\n"
	"        obj = Object.getPrototypeOf(Object(obj));\n"
	"    }\n"
	"    return { hints: res.join(''), color: 35, bold: 1 };\n"
	"})";

static duk_ret_t linenoise_add_completion(duk_context *ctx) {
	linenoiseCompletions *lc;

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "lc");
	lc = duk_require_pointer(ctx, -1);

	linenoiseAddCompletion(lc, duk_require_string(ctx, 0));
	return 0;
}

static char *linenoise_hints(const char *buf, int *color, int *bold) {
	duk_context *ctx;
	duk_int_t rc;

	ctx = completion_ctx;
	if (!ctx) {
		return NULL;
	}

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "linenoiseHints");
	if (!buf) {
		duk_push_undefined(ctx);
	} else {
		duk_push_string(ctx, buf);
	}

	rc = duk_pcall(ctx, 1 /*nargs*/);  /* [ stash func ] -> [ stash result ] */
	if (rc != 0) {
		const char *res;
		res = strdup(duk_safe_to_string(ctx, -1));
		*color = 31;  /* red */
		*bold = 1;
		duk_pop_2(ctx);
		return (char *) (uintptr_t) res;  /* uintptr_t cast to avoid const discard warning. */
	}

	if (duk_is_object(ctx, -1)) {
		const char *tmp;
		const char *res = NULL;

		duk_get_prop_string(ctx, -1, "hints");
		tmp = duk_get_string(ctx, -1);
		if (tmp) {
			res = strdup(tmp);
		}
		duk_pop(ctx);

		duk_get_prop_string(ctx, -1, "color");
		*color = duk_to_int(ctx, -1);
		duk_pop(ctx);

		duk_get_prop_string(ctx, -1, "bold");
		*bold = duk_to_int(ctx, -1);
		duk_pop(ctx);

		duk_pop_2(ctx);
		return (char *) (uintptr_t) res;  /* uintptr_t cast to avoid const discard warning. */
	}

	duk_pop_2(ctx);
	return NULL;
}

static void linenoise_freehints(void *ptr) {
#if 0
	printf("free hint: %p\n", (void *) ptr);
#endif
	free(ptr);
}

static void linenoise_completion(const char *buf, linenoiseCompletions *lc) {
	duk_context *ctx;
	duk_int_t rc;

	ctx = completion_ctx;
	if (!ctx) {
		return;
	}

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "linenoiseCompletion");

	if (!buf) {
		duk_push_undefined(ctx);
	} else {
		duk_push_string(ctx, buf);
	}
	duk_push_c_function(ctx, linenoise_add_completion, 2 /*nargs*/);
	duk_push_pointer(ctx, (void *) lc);
	duk_put_prop_string(ctx, -2, "lc");

	rc = duk_pcall(ctx, 2 /*nargs*/);  /* [ stash func callback ] -> [ stash result ] */
	if (rc != 0) {
		linenoiseAddCompletion(lc, duk_safe_to_string(ctx, -1));
	}
	duk_pop_2(ctx);
}
#endif  /* DUK_CMDLINE_LINENOISE_COMPLETION */

/*
 *  Execute from file handle etc
 */

static int handle_fh(duk_context *ctx, FILE *f, const char *filename, const char *bytecode_filename) {
	char *buf = NULL;
	size_t bufsz;
	size_t bufoff;
	size_t got;
	int rc;
	int retval = -1;

	buf = (char *) malloc(1024);
	if (!buf) {
		goto error;
	}
	bufsz = 1024;
	bufoff = 0;

	/* Read until EOF, avoid fseek/stat because it won't work with stdin. */
	for (;;) {
		size_t avail;

		avail = bufsz - bufoff;
		if (avail < 1024) {
			size_t newsz;
			char *buf_new;
#if 0
			fprintf(stderr, "resizing read buffer: %ld -> %ld\n", (long) bufsz, (long) (bufsz * 2));
#endif
			newsz = bufsz + (bufsz >> 2) + 1024;  /* +25% and some extra */
			if (newsz < bufsz) {
				goto error;
			}
			buf_new = (char *) realloc(buf, newsz);
			if (!buf_new) {
				goto error;
			}
			buf = buf_new;
			bufsz = newsz;
		}

		avail = bufsz - bufoff;
#if 0
		fprintf(stderr, "reading input: buf=%p bufsz=%ld bufoff=%ld avail=%ld\n",
		        (void *) buf, (long) bufsz, (long) bufoff, (long) avail);
#endif

		got = fread((void *) (buf + bufoff), (size_t) 1, avail, f);
#if 0
		fprintf(stderr, "got=%ld\n", (long) got);
#endif
		if (got == 0) {
			break;
		}
		bufoff += got;

		/* Emscripten specific: stdin EOF doesn't work as expected.
		 * Instead, when 'emduk' is executed using Node.js, a file
		 * piped to stdin repeats (!).  Detect that repeat and cut off
		 * the stdin read.  Ensure the loop repeats enough times to
		 * avoid detecting spurious loops.
		 *
		 * This only seems to work for inputs up to 256 bytes long.
		 */
#if defined(EMSCRIPTEN)
		if (bufoff >= 16384) {
			size_t i, j, nloops;
			int looped = 0;

			for (i = 16; i < bufoff / 8; i++) {
				int ok;

				nloops = bufoff / i;
				ok = 1;
				for (j = 1; j < nloops; j++) {
					if (memcmp((void *) buf, (void *) (buf + i * j), i) != 0) {
						ok = 0;
						break;
					}
				}
				if (ok) {
					fprintf(stderr, "emscripten workaround: detect looping at index %ld, verified with %ld loops\n", (long) i, (long) (nloops - 1));
					bufoff = i;
					looped = 1;
					break;
				}
			}

			if (looped) {
				break;
			}
		}
#endif
	}

	duk_push_string(ctx, bytecode_filename);
	duk_push_pointer(ctx, (void *) buf);
	duk_push_uint(ctx, (duk_uint_t) bufoff);
	duk_push_string(ctx, filename);

	interactive_mode = 0;  /* global */

	rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 4 /*nargs*/, 1 /*nret*/);

#if defined(DUK_CMDLINE_LOWMEM)
	lowmem_clear_exec_timeout();
#endif

	free(buf);
	buf = NULL;

	if (rc != DUK_EXEC_SUCCESS) {
		print_pop_error(ctx, stderr);
		goto error;
	} else {
		duk_pop(ctx);
		retval = 0;
	}
	/* fall thru */

 cleanup:
	if (buf) {
		free(buf);
		buf = NULL;
	}
	return retval;

 error:
	fprintf(stderr, "error in executing file %s\n", filename);
	fflush(stderr);
	goto cleanup;
}

static int handle_file(duk_context *ctx, const char *filename, const char *bytecode_filename) {
	FILE *f = NULL;
	int retval;
	char fnbuf[256];

	/* Example of sending an application specific debugger notification. */
	duk_push_string(ctx, "DebuggerHandleFile");
	duk_push_string(ctx, filename);
	duk_debugger_notify(ctx, 2);

#if defined(EMSCRIPTEN)
	if (filename[0] == '/') {
		snprintf(fnbuf, sizeof(fnbuf), "%s", filename);
	} else {
		snprintf(fnbuf, sizeof(fnbuf), "/working/%s", filename);
	}
#else
	snprintf(fnbuf, sizeof(fnbuf), "%s", filename);
#endif
	fnbuf[sizeof(fnbuf) - 1] = (char) 0;

	f = fopen(fnbuf, "rb");
	if (!f) {
		fprintf(stderr, "failed to open source file: %s\n", filename);
		fflush(stderr);
		goto error;
	}

	retval = handle_fh(ctx, f, filename, bytecode_filename);

	fclose(f);
	return retval;

 error:
	return -1;
}

static int handle_eval(duk_context *ctx, char *code) {
	int rc;
	int retval = -1;

	duk_push_pointer(ctx, (void *) code);
	duk_push_uint(ctx, (duk_uint_t) strlen(code));
	duk_push_string(ctx, "eval");

	interactive_mode = 0;  /* global */

	rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 3 /*nargs*/, 1 /*nret*/);

#if defined(DUK_CMDLINE_LOWMEM)
	lowmem_clear_exec_timeout();
#endif

	if (rc != DUK_EXEC_SUCCESS) {
		print_pop_error(ctx, stderr);
	} else {
		duk_pop(ctx);
		retval = 0;
	}

	return retval;
}

#if defined(DUK_CMDLINE_LINENOISE)
static int handle_interactive(duk_context *ctx) {
	const char *prompt = "duk> ";
	char *buffer = NULL;
	int retval = 0;
	int rc;

	linenoiseSetMultiLine(1);
	linenoiseHistorySetMaxLen(64);
#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
	if (!no_auto_complete) {
		linenoiseSetCompletionCallback(linenoise_completion);
		linenoiseSetHintsCallback(linenoise_hints);
		linenoiseSetFreeHintsCallback(linenoise_freehints);
		duk_push_global_stash(ctx);
		duk_eval_string(ctx, linenoise_completion_script);
		duk_put_prop_string(ctx, -2, "linenoiseCompletion");
		duk_eval_string(ctx, linenoise_hints_script);
		duk_put_prop_string(ctx, -2, "linenoiseHints");
		duk_pop(ctx);
	}
#endif

	for (;;) {
		if (buffer) {
			linenoiseFree(buffer);
			buffer = NULL;
		}

#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
		completion_ctx = ctx;
#endif
		buffer = linenoise(prompt);
#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
		completion_ctx = NULL;
#endif

		if (!buffer) {
			break;
		}

		if (buffer && buffer[0] != (char) 0) {
			linenoiseHistoryAdd(buffer);
		}

		duk_push_pointer(ctx, (void *) buffer);
		duk_push_uint(ctx, (duk_uint_t) strlen(buffer));
		duk_push_string(ctx, "input");

		interactive_mode = 1;  /* global */

		rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 3 /*nargs*/, 1 /*nret*/);

#if defined(DUK_CMDLINE_LOWMEM)
		lowmem_clear_exec_timeout();
#endif

		if (buffer) {
			linenoiseFree(buffer);
			buffer = NULL;
		}

		if (rc != DUK_EXEC_SUCCESS) {
			/* in interactive mode, write to stdout */
			print_pop_error(ctx, stdout);
			retval = -1;  /* an error 'taints' the execution */
		} else {
			duk_pop(ctx);
		}
	}

	if (buffer) {
		linenoiseFree(buffer);
		buffer = NULL;
	}

	return retval;
}
#else  /* DUK_CMDLINE_LINENOISE */
static int handle_interactive(duk_context *ctx) {
	const char *prompt = "duk> ";
	size_t bufsize = 0;
	char *buffer = NULL;
	int retval = 0;
	int rc;
	int got_eof = 0;

	while (!got_eof) {
		size_t idx = 0;

		fwrite(prompt, 1, strlen(prompt), stdout);
		fflush(stdout);

		for (;;) {
			int c;

			if (idx >= bufsize) {
				size_t newsize = bufsize + (bufsize >> 2) + 1024;  /* +25% and some extra */
				char *newptr;

				if (newsize < bufsize) {
					goto fail_realloc;
				}
				newptr = (char *) realloc(buffer, newsize);
				if (!newptr) {
					goto fail_realloc;
				}
				buffer = newptr;
				bufsize = newsize;
			}

			c = fgetc(stdin);
			if (c == EOF) {
				got_eof = 1;
				break;
			} else if (c == '\n') {
				break;
			} else {
				buffer[idx++] = (char) c;
			}
		}

		duk_push_pointer(ctx, (void *) buffer);
		duk_push_uint(ctx, (duk_uint_t) idx);
		duk_push_string(ctx, "input");

		interactive_mode = 1;  /* global */

		rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 3 /*nargs*/, 1 /*nret*/);

#if defined(DUK_CMDLINE_LOWMEM)
		lowmem_clear_exec_timeout();
#endif

		if (rc != DUK_EXEC_SUCCESS) {
			/* in interactive mode, write to stdout */
			print_pop_error(ctx, stdout);
			retval = -1;  /* an error 'taints' the execution */
		} else {
			duk_pop(ctx);
		}
	}

 done:
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}

	return retval;

 fail_realloc:
	fprintf(stderr, "failed to extend line buffer\n");
	fflush(stderr);
	retval = -1;
	goto done;
}
#endif  /* DUK_CMDLINE_LINENOISE */

/*
 *  Simple file read/write bindings
 */

#if defined(DUK_CMDLINE_FILEIO)
static duk_ret_t fileio_read_file(duk_context *ctx) {
	const char *fn;
	char *buf;
	size_t len;
	size_t off;
	int rc;
	FILE *f;

	fn = duk_require_string(ctx, 0);
	f = fopen(fn, "rb");
	if (!f) {
		(void) duk_type_error(ctx, "cannot open file %s for reading, errno %ld: %s",
		                      fn, (long) errno, strerror(errno));
	}

	rc = fseek(f, 0, SEEK_END);
	if (rc < 0) {
		(void) fclose(f);
		(void) duk_type_error(ctx, "fseek() failed for %s, errno %ld: %s",
		                      fn, (long) errno, strerror(errno));
	}
	len = (size_t) ftell(f);
	rc = fseek(f, 0, SEEK_SET);
	if (rc < 0) {
		(void) fclose(f);
		(void) duk_type_error(ctx, "fseek() failed for %s, errno %ld: %s",
		                      fn, (long) errno, strerror(errno));
	}

	buf = (char *) duk_push_fixed_buffer(ctx, (duk_size_t) len);
	for (off = 0; off < len;) {
		size_t got;
		got = fread((void *) (buf + off), 1, len - off, f);
		if (ferror(f)) {
			(void) fclose(f);
			(void) duk_type_error(ctx, "error while reading %s", fn);
		}
		if (got == 0) {
			if (feof(f)) {
				break;
			} else {
				(void) fclose(f);
				(void) duk_type_error(ctx, "error while reading %s", fn);
			}
		}
		off += got;
	}

	if (f) {
		(void) fclose(f);
	}

	return 1;
}

static duk_ret_t fileio_write_file(duk_context *ctx) {
	const char *fn;
	const char *buf;
	size_t len;
	size_t off;
	FILE *f;

	fn = duk_require_string(ctx, 0);
	f = fopen(fn, "wb");
	if (!f) {
		(void) duk_type_error(ctx, "cannot open file %s for writing, errno %ld: %s",
		          fn, (long) errno, strerror(errno));
	}

	len = 0;
	buf = (char *) duk_require_buffer_data(ctx, 1, &len);
	for (off = 0; off < len;) {
		size_t got;
		got = fwrite((const void *) (buf + off), 1, len - off, f);
		if (ferror(f)) {
			(void) fclose(f);
			(void) duk_type_error(ctx, "error while writing %s", fn);
		}
		if (got == 0) {
			(void) fclose(f);
			(void) duk_type_error(ctx, "error while writing %s", fn);
		}
		off += got;
	}

	if (f) {
		(void) fclose(f);
	}

	return 0;
}
#endif  /* DUK_CMDLINE_FILEIO */

/*
 *  String.fromBufferRaw()
 */

static duk_ret_t string_frombufferraw(duk_context *ctx) {
	duk_buffer_to_string(ctx, 0);
	return 1;
}

/*
 *  Duktape heap lifecycle
 */

#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
static duk_idx_t debugger_request(duk_context *ctx, void *udata, duk_idx_t nvalues) {
	const char *cmd;
	int i;

	(void) udata;

	if (nvalues < 1) {
		duk_push_string(ctx, "missing AppRequest argument(s)");
		return -1;
	}

	cmd = duk_get_string(ctx, -nvalues + 0);

	if (cmd && strcmp(cmd, "CommandLine") == 0) {
		if (!duk_check_stack(ctx, main_argc)) {
			/* Callback should avoid errors for now, so use
			 * duk_check_stack() rather than duk_require_stack().
			 */
			duk_push_string(ctx, "failed to extend stack");
			return -1;
		}
		for (i = 0; i < main_argc; i++) {
			duk_push_string(ctx, main_argv[i]);
		}
		return main_argc;
	}
	duk_push_sprintf(ctx, "command not supported");
	return -1;
}

static void debugger_detached(duk_context *ctx, void *udata) {
	fprintf(stderr, "Debugger detached, udata: %p\n", (void *) udata);
	fflush(stderr);

	/* Ensure socket is closed even when detach is initiated by Duktape
	 * rather than debug client.
	 */
	duk_trans_socket_finish();

	if (debugger_reattach) {
		/* For automatic reattach testing. */
		duk_trans_socket_init();
		duk_trans_socket_waitconn();
		fprintf(stderr, "Debugger reconnected, call duk_debugger_attach()\n");
		fflush(stderr);
#if 0
		/* This is not necessary but should be harmless. */
		duk_debugger_detach(ctx);
#endif
		duk_debugger_attach(ctx,
		                    duk_trans_socket_read_cb,
		                    duk_trans_socket_write_cb,
		                    duk_trans_socket_peek_cb,
		                    duk_trans_socket_read_flush_cb,
		                    duk_trans_socket_write_flush_cb,
		                    debugger_request,
		                    debugger_detached,
		                    NULL);
	}
}
#endif

#define  ALLOC_DEFAULT  0
#define  ALLOC_LOGGING  1
#define  ALLOC_TORTURE  2
#define  ALLOC_HYBRID   3
#define  ALLOC_LOWMEM   4

static duk_context *create_duktape_heap(int alloc_provider, int debugger, int lowmem_log) {
	duk_context *ctx;

	(void) lowmem_log;  /* suppress warning */

	ctx = NULL;
	if (!ctx && alloc_provider == ALLOC_LOGGING) {
#if defined(DUK_CMDLINE_ALLOC_LOGGING)
		ctx = duk_create_heap(duk_alloc_logging,
		                      duk_realloc_logging,
		                      duk_free_logging,
		                      (void *) 0xdeadbeef,
		                      cmdline_fatal_handler);
#else
		fprintf(stderr, "Warning: option --alloc-logging ignored, no logging allocator support\n");
		fflush(stderr);
#endif
	}
	if (!ctx && alloc_provider == ALLOC_TORTURE) {
#if defined(DUK_CMDLINE_ALLOC_TORTURE)
		ctx = duk_create_heap(duk_alloc_torture,
		                      duk_realloc_torture,
		                      duk_free_torture,
		                      (void *) 0xdeadbeef,
		                      cmdline_fatal_handler);
#else
		fprintf(stderr, "Warning: option --alloc-torture ignored, no torture allocator support\n");
		fflush(stderr);
#endif
	}
	if (!ctx && alloc_provider == ALLOC_HYBRID) {
#if defined(DUK_CMDLINE_ALLOC_HYBRID)
		void *udata = duk_alloc_hybrid_init();
		if (!udata) {
			fprintf(stderr, "Failed to init hybrid allocator\n");
			fflush(stderr);
		} else {
			ctx = duk_create_heap(duk_alloc_hybrid,
			                      duk_realloc_hybrid,
			                      duk_free_hybrid,
			                      udata,
			                      cmdline_fatal_handler);
		}
#else
		fprintf(stderr, "Warning: option --alloc-hybrid ignored, no hybrid allocator support\n");
		fflush(stderr);
#endif
	}
	if (!ctx && alloc_provider == ALLOC_LOWMEM) {
#if defined(DUK_CMDLINE_LOWMEM)
		lowmem_init();

		ctx = duk_create_heap(
			lowmem_log ? lowmem_alloc_wrapped : duk_alloc_pool,
			lowmem_log ? lowmem_realloc_wrapped : duk_realloc_pool,
			lowmem_log ? lowmem_free_wrapped : duk_free_pool,
			(void *) lowmem_pool_ptr,
			cmdline_fatal_handler);
#else
		fprintf(stderr, "Warning: option --alloc-ajsheap ignored, no ajsheap allocator support\n");
		fflush(stderr);
#endif
	}
	if (!ctx && alloc_provider == ALLOC_DEFAULT) {
		ctx = duk_create_heap(NULL, NULL, NULL, NULL, cmdline_fatal_handler);
	}

	if (!ctx) {
		fprintf(stderr, "Failed to create Duktape heap\n");
		fflush(stderr);
		exit(1);
	}

#if defined(DUK_CMDLINE_LOWMEM)
	if (alloc_provider == ALLOC_LOWMEM) {
		fprintf(stderr, "*** pool dump after heap creation ***\n");
		lowmem_dump();
	}
#endif

#if defined(DUK_CMDLINE_LOWMEM)
	if (alloc_provider == ALLOC_LOWMEM) {
		lowmem_register(ctx);
	}
#endif

	/* Register print() and alert() (removed in Duktape 2.x). */
#if defined(DUK_CMDLINE_PRINTALERT_SUPPORT)
	duk_print_alert_init(ctx, 0 /*flags*/);
#endif

	/* Register String.fromBufferRaw() which does a 1:1 buffer-to-string
	 * coercion needed by testcases.  String.fromBufferRaw() is -not- a
	 * default built-in!  For stripped builds the 'String' built-in
	 * doesn't exist and we create it here; for ROM builds it may be
	 * present but unwritable (which is ignored).
	 */
	duk_eval_string(ctx,
		"(function(v){"
		    "if (typeof String === 'undefined') { String = {}; }"
		    "Object.defineProperty(String, 'fromBufferRaw', {value:v, configurable:true});"
		"})");
	duk_push_c_function(ctx, string_frombufferraw, 1 /*nargs*/);
	(void) duk_pcall(ctx, 1);
	duk_pop(ctx);

	/* Register console object. */
#if defined(DUK_CMDLINE_CONSOLE_SUPPORT)
	duk_console_init(ctx, DUK_CONSOLE_FLUSH /*flags*/);
#endif

	/* Register Duktape.Logger (removed in Duktape 2.x). */
#if defined(DUK_CMDLINE_LOGGING_SUPPORT)
	duk_logging_init(ctx, 0 /*flags*/);
#endif

	/* Register require() (removed in Duktape 2.x). */
#if defined(DUK_CMDLINE_MODULE_SUPPORT)
	duk_module_duktape_init(ctx);
#endif

	/* Trivial readFile/writeFile bindings for testing. */
#if defined(DUK_CMDLINE_FILEIO)
	duk_push_c_function(ctx, fileio_read_file, 1 /*nargs*/);
	duk_put_global_string(ctx, "readFile");
	duk_push_c_function(ctx, fileio_write_file, 2 /*nargs*/);
	duk_put_global_string(ctx, "writeFile");
#endif

	/* Stash a formatting function for evaluation results. */
	duk_push_global_stash(ctx);
	duk_eval_string(ctx,
		"(function (E) {"
		    "return function format(v){"
		        "try{"
		            "return E('jx',v);"
		        "}catch(e){"
		            "return ''+v;"
		        "}"
		    "};"
		"})(Duktape.enc)");
	duk_put_prop_string(ctx, -2, "dukFormat");
	duk_pop(ctx);

	if (debugger) {
#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
		fprintf(stderr, "Debugger enabled, create socket and wait for connection\n");
		fflush(stderr);
		duk_trans_socket_init();
		duk_trans_socket_waitconn();
		fprintf(stderr, "Debugger connected, call duk_debugger_attach() and then execute requested file(s)/eval\n");
		fflush(stderr);
		duk_debugger_attach(ctx,
		                    duk_trans_socket_read_cb,
		                    duk_trans_socket_write_cb,
		                    duk_trans_socket_peek_cb,
		                    duk_trans_socket_read_flush_cb,
		                    duk_trans_socket_write_flush_cb,
		                    debugger_request,
		                    debugger_detached,
		                    NULL);
#else
		fprintf(stderr, "Warning: option --debugger ignored, no debugger support\n");
		fflush(stderr);
#endif
	}

#if 0
	/* Manual test for duk_debugger_cooperate() */
	{
		for (i = 0; i < 60; i++) {
			printf("cooperate: %d\n", i);
			usleep(1000000);
			duk_debugger_cooperate(ctx);
		}
	}
#endif

	return ctx;
}

static void destroy_duktape_heap(duk_context *ctx, int alloc_provider) {
	(void) alloc_provider;

#if defined(DUK_CMDLINE_LOWMEM)
	if (alloc_provider == ALLOC_LOWMEM) {
		fprintf(stderr, "*** pool dump before duk_destroy_heap(), before forced gc ***\n");
		lowmem_dump();

		duk_gc(ctx, 0);

		fprintf(stderr, "*** pool dump before duk_destroy_heap(), after forced gc ***\n");
		lowmem_dump();
	}
#endif

	if (ctx) {
		duk_destroy_heap(ctx);
	}

#if defined(DUK_CMDLINE_LOWMEM)
	if (alloc_provider == ALLOC_LOWMEM) {
		fprintf(stderr, "*** pool dump after duk_destroy_heap() (should have zero allocs) ***\n");
		lowmem_dump();
	}
	lowmem_free();
#endif
}

/*
 *  Main
 */

int main(int argc, char *argv[]) {
	duk_context *ctx = NULL;
	int retval = 0;
	int have_files = 0;
	int have_eval = 0;
	int interactive = 0;
	int memlimit_high = 1;
	int alloc_provider = ALLOC_DEFAULT;
	int lowmem_log = 0;
	int debugger = 0;
	int recreate_heap = 0;
	int no_heap_destroy = 0;
	int verbose = 0;
	int run_stdin = 0;
	const char *compile_filename = NULL;
	int i;

	main_argc = argc;
	main_argv = (char **) argv;

#if defined(EMSCRIPTEN)
	/* Try to use NODEFS to provide access to local files.  Mount the
	 * CWD as /working, and then prepend "/working/" to relative native
	 * paths in file calls to get something that works reasonably for
	 * relative paths.  Emscripten doesn't support replacing virtual
	 * "/" with host "/" (the default MEMFS at "/" can't be unmounted)
	 * but we can mount "/tmp" as host "/tmp" to allow testcase runs.
	 *
	 * https://kripken.github.io/emscripten-site/docs/api_reference/Filesystem-API.html#filesystem-api-nodefs
	 * https://github.com/kripken/emscripten/blob/master/tests/fs/test_nodefs_rw.c
	 */
	EM_ASM(
		/* At the moment it's not possible to replace the default MEMFS mounted at '/':
		 * https://github.com/kripken/emscripten/issues/2040
		 * https://github.com/kripken/emscripten/blob/incoming/src/library_fs.js#L1341-L1358
		 */
		/*
		try {
			FS.unmount("/");
		} catch (e) {
			console.log("Failed to unmount default '/' MEMFS mount: " + e);
		}
		*/
		try {
			FS.mkdir("/working");
			FS.mount(NODEFS, { root: "." }, "/working");
		} catch (e) {
			console.log("Failed to mount NODEFS /working: " + e);
		}
		/* A virtual '/tmp' exists by default:
		 * https://gist.github.com/evanw/e6be28094f34451bd5bd#file-temp-js-L3806-L3809
		 */
		/*
		try {
			FS.mkdir("/tmp");
		} catch (e) {
			console.log("Failed to create virtual /tmp: " + e);
		}
		*/
		try {
			FS.mount(NODEFS, { root: "/tmp" }, "/tmp");
		} catch (e) {
			console.log("Failed to mount NODEFS /tmp: " + e);
		}
	);
#endif  /* EMSCRIPTEN */

#if defined(DUK_CMDLINE_LOWMEM)
	alloc_provider = ALLOC_LOWMEM;
#endif
	(void) lowmem_log;

	/*
	 *  Signal handling setup
	 */

#if defined(DUK_CMDLINE_SIGNAL)
	set_sigint_handler();

	/* This is useful at the global level; libraries should avoid SIGPIPE though */
	/*signal(SIGPIPE, SIG_IGN);*/
#endif

	/*
	 *  Parse options
	 */

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!arg) {
			goto usage;
		}
		if (strcmp(arg, "--restrict-memory") == 0) {
			memlimit_high = 0;
		} else if (strcmp(arg, "-i") == 0) {
			interactive = 1;
		} else if (strcmp(arg, "-b") == 0) {
			allow_bytecode = 1;
		} else if (strcmp(arg, "-c") == 0) {
			if (i == argc - 1) {
				goto usage;
			}
			i++;
			compile_filename = argv[i];
		} else if (strcmp(arg, "-e") == 0) {
			have_eval = 1;
			if (i == argc - 1) {
				goto usage;
			}
			i++;  /* skip code */
		} else if (strcmp(arg, "--alloc-default") == 0) {
			alloc_provider = ALLOC_DEFAULT;
		} else if (strcmp(arg, "--alloc-logging") == 0) {
			alloc_provider = ALLOC_LOGGING;
		} else if (strcmp(arg, "--alloc-torture") == 0) {
			alloc_provider = ALLOC_TORTURE;
		} else if (strcmp(arg, "--alloc-hybrid") == 0) {
			alloc_provider = ALLOC_HYBRID;
		} else if (strcmp(arg, "--alloc-lowmem") == 0) {
			alloc_provider = ALLOC_LOWMEM;
		} else if (strcmp(arg, "--lowmem-log") == 0) {
			lowmem_log = 1;
		} else if (strcmp(arg, "--debugger") == 0) {
			debugger = 1;
#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
		} else if (strcmp(arg, "--reattach") == 0) {
			debugger_reattach = 1;
#endif
		} else if (strcmp(arg, "--recreate-heap") == 0) {
			recreate_heap = 1;
		} else if (strcmp(arg, "--no-heap-destroy") == 0) {
			no_heap_destroy = 1;
		} else if (strcmp(arg, "--no-auto-complete") == 0) {
#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
			no_auto_complete = 1;
#endif
		} else if (strcmp(arg, "--verbose") == 0) {
			verbose = 1;
		} else if (strcmp(arg, "--run-stdin") == 0) {
			run_stdin = 1;
		} else if (strlen(arg) >= 1 && arg[0] == '-') {
			goto usage;
		} else {
			have_files = 1;
		}
	}
	if (!have_files && !have_eval && !run_stdin) {
		interactive = 1;
	}

	/*
	 *  Memory limit
	 */

#if defined(DUK_CMDLINE_RLIMIT)
	set_resource_limits(memlimit_high ? MEM_LIMIT_HIGH : MEM_LIMIT_NORMAL);
#else
	if (memlimit_high == 0) {
		fprintf(stderr, "Warning: option --restrict-memory ignored, no rlimit support\n");
		fflush(stderr);
	}
#endif

	/*
	 *  Create heap
	 */

	ctx = create_duktape_heap(alloc_provider, debugger, lowmem_log);

	/*
	 *  Execute any argument file(s)
	 */

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!arg) {
			continue;
		} else if (strlen(arg) == 2 && strcmp(arg, "-e") == 0) {
			/* Here we know the eval arg exists but check anyway */
			if (i == argc - 1) {
				retval = 1;
				goto cleanup;
			}
			if (handle_eval(ctx, argv[i + 1]) != 0) {
				retval = 1;
				goto cleanup;
			}
			i++;  /* skip code */
			continue;
		} else if (strlen(arg) == 2 && strcmp(arg, "-c") == 0) {
			i++;  /* skip filename */
			continue;
		} else if (strlen(arg) >= 1 && arg[0] == '-') {
			continue;
		}

		if (verbose) {
			fprintf(stderr, "*** Executing file: %s\n", arg);
			fflush(stderr);
		}

		if (handle_file(ctx, arg, compile_filename) != 0) {
			retval = 1;
			goto cleanup;
		}

		if (recreate_heap) {
			if (verbose) {
				fprintf(stderr, "*** Recreating heap...\n");
				fflush(stderr);
			}

			destroy_duktape_heap(ctx, alloc_provider);
			ctx = create_duktape_heap(alloc_provider, debugger, lowmem_log);
		}
	}

	if (run_stdin) {
		/* Running stdin like a full file (reading all lines before
		 * compiling) is useful with emduk:
		 * cat test.js | ./emduk --run-stdin
		 */
		if (handle_fh(ctx, stdin, "stdin", compile_filename) != 0) {
			retval = 1;
			goto cleanup;
		}

		if (recreate_heap) {
			if (verbose) {
				fprintf(stderr, "*** Recreating heap...\n");
				fflush(stderr);
			}

			destroy_duktape_heap(ctx, alloc_provider);
			ctx = create_duktape_heap(alloc_provider, debugger, lowmem_log);
		}
	}

	/*
	 *  Enter interactive mode if options indicate it
	 */

	if (interactive) {
		print_greet_line();
		if (handle_interactive(ctx) != 0) {
			retval = 1;
			goto cleanup;
		}
	}

	/*
	 *  Cleanup and exit
	 */

 cleanup:
	if (interactive) {
		fprintf(stderr, "Cleaning up...\n");
		fflush(stderr);
	}

	if (ctx && no_heap_destroy) {
		duk_gc(ctx, 0);
	}
	if (ctx && !no_heap_destroy) {
		destroy_duktape_heap(ctx, alloc_provider);
	}
	ctx = NULL;

	return retval;

	/*
	 *  Usage
	 */

 usage:
	fprintf(stderr, "Usage: duk [options] [<filenames>]\n"
	                "\n"
	                "   -i                 enter interactive mode after executing argument file(s) / eval code\n"
	                "   -e CODE            evaluate code\n"
	                "   -c FILE            compile into bytecode and write to FILE (use with only one file argument)\n"
	                "   -b                 allow bytecode input files (memory unsafe for invalid bytecode)\n"
	                "   --run-stdin        treat stdin like a file, i.e. compile full input (not line by line)\n"
	                "   --verbose          verbose messages to stderr\n"
	                "   --restrict-memory  use lower memory limit (used by test runner)\n"
	                "   --alloc-default    use Duktape default allocator\n"
#if defined(DUK_CMDLINE_ALLOC_LOGGING)
	                "   --alloc-logging    use logging allocator, write alloc log to /tmp/duk-alloc-log.txt\n"
#endif
#if defined(DUK_CMDLINE_ALLOC_TORTURE)
	                "   --alloc-torture    use torture allocator\n"
#endif
#if defined(DUK_CMDLINE_ALLOC_HYBRID)
	                "   --alloc-hybrid     use hybrid allocator\n"
#endif
#if defined(DUK_CMDLINE_LOWMEM)
	                "   --alloc-lowmem     use pooled allocator (enabled by default for duk-low)\n"
	                "   --lowmem-log       write alloc log to /tmp/lowmem-alloc-log.txt\n"
#endif
#if defined(DUK_CMDLINE_DEBUGGER_SUPPORT)
	                "   --debugger         start example debugger\n"
	                "   --reattach         automatically reattach debugger on detach\n"
#endif
	                "   --recreate-heap    recreate heap after every file\n"
	                "   --no-heap-destroy  force GC, but don't destroy heap at end (leak testing)\n"
#if defined(DUK_CMDLINE_LINENOISE_COMPLETION)
	                "   --no-auto-complete disable linenoise auto completion\n"
#else
	                "   --no-auto-complete disable linenoise auto completion [ignored, not supported]\n"
#endif
	                "\n"
	                "If <filename> is omitted, interactive mode is started automatically.\n"
	                "\n"
	                "Input files can be either ECMAScript source files or bytecode files\n"
	                "(if -b is given).  Bytecode files are not validated prior to loading,\n"
	                "so that incompatible or crafted files can cause memory unsafe behavior.\n"
	                "See discussion in\n"
	                "https://github.com/svaarala/duktape/blob/master/doc/bytecode.rst#memory-safety-and-bytecode-validation.\n");
	fflush(stderr);
	exit(1);
}

/* Example of how a native stack check can be implemented in a platform
 * specific manner for DUK_USE_NATIVE_STACK_CHECK().  This example is for
 * (Linux) pthreads, and rejects further native recursion if less than
 * 16kB stack is left (conservative).
 */
#if defined(DUK_CMDLINE_PTHREAD_STACK_CHECK)
int duk_cmdline_stack_check(void) {
	pthread_attr_t attr;
	void *stackaddr;
	size_t stacksize;
	char *ptr;
	char *ptr_base;
	ptrdiff_t remain;

	(void) pthread_getattr_np(pthread_self(), &attr);
	(void) pthread_attr_getstack(&attr, &stackaddr, &stacksize);
	ptr = (char *) &stacksize;  /* Rough estimate of current stack pointer. */
	ptr_base = (char *) stackaddr;
	remain = ptr - ptr_base;

	/* HIGH ADDR   -----  stackaddr + stacksize
	 *               |
	 *               |  stack growth direction
	 *               v -- ptr, approximate used size
	 *
	 * LOW ADDR    -----  ptr_base, end of stack (lowest address)
	 */

#if 0
	fprintf(stderr, "STACK CHECK: stackaddr=%p, stacksize=%ld, ptr=%p, remain=%ld\n",
	        stackaddr, (long) stacksize, (void *) ptr, (long) remain);
	fflush(stderr);
#endif
	if (remain < 16384) {
		return 1;
	}
	return 0;  /* 0: no error, != 0: throw RangeError */
}
#else
int duk_cmdline_stack_check(void) {
	return 0;
}
#endif
