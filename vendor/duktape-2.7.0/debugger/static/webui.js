/*
 *  Duktape debugger web client
 *
 *  Talks to the NodeJS server using socket.io.
 *
 *  http://unixpapa.com/js/key.html
 */

// Update interval for custom source highlighting.
var SOURCE_UPDATE_INTERVAL = 350;

// Source view
var activeFileName = null;          // file that we want to be loaded in source view
var activeLine = null;              // scroll to line once file has been loaded
var activeHighlight = null;         // line that we want to highlight (if any)
var loadedFileName = null;          // currently loaded (shown) file
var loadedLineCount = 0;            // currently loaded file line count
var loadedFileExecuting = false;    // true if currFileName (loosely) matches loadedFileName
var loadedLinePending = null;       // if set, scroll loaded file to requested line
var highlightLine = null;           // highlight line
var sourceEditedLines = [];         // line numbers which have been modified
                                    // (added classes etc, tracked for removing)
var sourceUpdateInterval = null;    // timer for updating source view
var sourceFetchXhr = null;          // current AJAX request for fetching a source file (if any)
var forceButtonUpdate = false;      // hack to reset button states
var bytecodeDialogOpen = false;     // bytecode dialog active
var bytecodeIdxHighlight = null;    // index of currently highlighted line (or null)
var bytecodeIdxInstr = 0;           // index to first line of bytecode instructions

// Execution state
var prevState = null;               // previous execution state ('paused', 'running', etc)
var prevAttached = null;            // previous debugger attached state (true, false, null)
var currFileName = null;            // current filename being executed
var currFuncName = null;            // current function name being executed
var currLine = 0;                   // current line being executed
var currPc = 0;                     // current bytecode PC being executed
var currState = 0;                  // current execution state ('paused', 'running', 'detached', etc)
var currAttached = false;           // current debugger attached state (true or false)
var currLocals = [];                // current local variables
var currCallstack = [];             // current callstack (from top to bottom)
var currBreakpoints = [];           // current breakpoints
var startedRunning = 0;             // timestamp when last started running (if running)
                                    // (used to grey out the source file if running for long enough)

/*
 *  Helpers
 */

function formatBytes(x) {
    if (x < 1024) {
        return String(x) + ' bytes';
    } else if (x < 1024 * 1024) {
        return (x / 1024).toPrecision(3) + ' kB';
    } else {
        return (x / (1024 * 1024)).toPrecision(3) + ' MB';
    }
}

/*
 *  Source view periodic update handling
 */

function doSourceUpdate() {
    var elem;

    // Remove previously added custom classes
    sourceEditedLines.forEach(function (linenum) {
        elem = $('#source-code div')[linenum - 1];
        if (elem) {
            elem.classList.remove('breakpoint');
            elem.classList.remove('execution');
            elem.classList.remove('highlight');
        }
    });
    sourceEditedLines.length = 0;

    // If we're executing the file shown, highlight current line
    if (loadedFileExecuting) {
        elem = $('#source-code div')[currLine - 1];
        if (elem) {
            sourceEditedLines.push(currLine);
            elem.classList.add('execution');
        }
    }

    // Add breakpoints
    currBreakpoints.forEach(function (bp) {
        if (bp.fileName === loadedFileName) {
            elem = $('#source-code div')[bp.lineNumber - 1];
            if (elem) {
                sourceEditedLines.push(bp.lineNumber);
                elem.classList.add('breakpoint');
            }
        }
    });

    if (highlightLine !== null) {
        elem = $('#source-code div')[highlightLine - 1];
        if (elem) {
            sourceEditedLines.push(highlightLine);
            elem.classList.add('highlight');
        }
    }

    // Bytecode dialog highlight
    if (loadedFileExecuting && bytecodeDialogOpen && bytecodeIdxHighlight !== bytecodeIdxInstr + currPc) {
        if (typeof bytecodeIdxHighlight === 'number') {
            $('#bytecode-preformatted div')[bytecodeIdxHighlight].classList.remove('highlight');
        }
        bytecodeIdxHighlight = bytecodeIdxInstr + currPc;
        $('#bytecode-preformatted div')[bytecodeIdxHighlight].classList.add('highlight');
    }

    // If no-one requested us to scroll to a specific line, finish.
    if (loadedLinePending == null) {
        return;
    }

    var reqLine = loadedLinePending;
    loadedLinePending = null;

    // Scroll to requested line.  This is not very clean, so a better solution
    // should be found:
    // https://developer.mozilla.org/en-US/docs/Web/API/Element.scrollIntoView
    // http://erraticdev.blogspot.fi/2011/02/jquery-scroll-into-view-plugin-with.html
    // http://flesler.blogspot.fi/2007/10/jqueryscrollto.html
    var tmpLine = Math.max(reqLine - 5, 0);
    elem = $('#source-code div')[tmpLine];
    if (elem) {
        elem.scrollIntoView();
    }
}

// Source is updated periodically.  Other code can also call doSourceUpdate()
// directly if an immediate update is needed.
sourceUpdateInterval = setInterval(doSourceUpdate, SOURCE_UPDATE_INTERVAL);

/*
 *  UI update handling when exec-status update arrives
 */

function doUiUpdate() {
    var now = Date.now();

    // Note: loadedFileName can be either from target or from server, but they
    // must match exactly.  We could do a loose match here, but exact matches
    // are needed for proper breakpoint handling anyway.
    loadedFileExecuting = (loadedFileName === currFileName);

    // If we just started running, store a timestamp so we can grey out the
    // source view only if we execute long enough (i.e. we're not just
    // stepping).
    if (currState !== prevState && currState === 'running') {
        startedRunning = now;
    }

    // If we just became paused, check for eval watch
    if (currState !== prevState && currState === 'paused') {
        if ($('#eval-watch').is(':checked')) {
            submitEval();  // don't clear eval input
        }
    }

    // Update current execution state
    if (currFileName === '' && currLine === 0) {
        $('#current-fileline').text('');
    } else {
        $('#current-fileline').text(String(currFileName) + ':' + String(currLine));
    }
    if (currFuncName === '' && currPc === 0) {
        $('#current-funcpc').text('');
    } else {
        $('#current-funcpc').text(String(currFuncName) + '() pc ' + String(currPc));
    }
    $('#current-state').text(String(currState));

    // Update buttons
    if (currState !== prevState || currAttached !== prevAttached || forceButtonUpdate) {
        $('#stepinto-button').prop('disabled', !currAttached || currState !== 'paused');
        $('#stepover-button').prop('disabled', !currAttached || currState !== 'paused');
        $('#stepout-button').prop('disabled', !currAttached || currState !== 'paused');
        $('#resume-button').prop('disabled', !currAttached || currState !== 'paused');
        $('#pause-button').prop('disabled', !currAttached || currState !== 'running');
        $('#attach-button').prop('disabled', currAttached);
        if (currAttached) {
            $('#attach-button').removeClass('enabled');
        } else {
            $('#attach-button').addClass('enabled');
        }
        $('#detach-button').prop('disabled', !currAttached);
        $('#eval-button').prop('disabled', !currAttached);
        $('#add-breakpoint-button').prop('disabled', !currAttached);
        $('#delete-all-breakpoints-button').prop('disabled', !currAttached);
        $('.delete-breakpoint-button').prop('disabled', !currAttached);
        $('#putvar-button').prop('disabled', !currAttached);
        $('#getvar-button').prop('disabled', !currAttached);
        $('#heap-dump-download-button').prop('disabled', !currAttached);
    }
    if (currState !== 'running' || forceButtonUpdate) {
        // Remove pending highlight once we're no longer running.
        $('#pause-button').removeClass('pending');
        $('#eval-button').removeClass('pending');
    }
    forceButtonUpdate = false;

    // Make source window grey when running for a longer time, use a small
    // delay to avoid flashing grey when stepping.
    if (currState === 'running' && now - startedRunning >= 500) {
        $('#source-pre').removeClass('notrunning');
        $('#current-state').removeClass('notrunning');
    } else {
        $('#source-pre').addClass('notrunning');
        $('#current-state').addClass('notrunning');
    }

    // Force source view to match currFileName only when running or when
    // just became paused (from running or detached).
    var fetchSource = false;
    if (typeof currFileName === 'string') {
        if (currState === 'running' ||
            (prevState !== 'paused' && currState === 'paused') ||
            (currAttached !== prevAttached)) {
            if (activeFileName !== currFileName) {
                fetchSource = true;
                activeFileName = currFileName;
                activeLine = currLine;
                activeHighlight = null;
                requestSourceRefetch();
            }
        }
    }

    // Force line update (scrollTop) only when running or just became paused.
    // Otherwise let user browse and scroll source files freely.
    if (!fetchSource) {
        if ((prevState !== 'paused' && currState === 'paused') ||
            currState === 'running') {
            loadedLinePending = currLine || 0;
        }
    }
}

/*
 *  Init socket.io and add handlers
 */

var socket = io();  // returns a Manager

setInterval(function () {
    socket.emit('keepalive', {
        userAgent: (navigator || {}).userAgent
    });
}, 30000);

socket.on('connect', function () {
    $('#socketio-info').text('connected');
    currState = 'connected';

    fetchSourceList();
});
socket.on('disconnect', function () {
    $('#socketio-info').text('not connected');
    currState = 'disconnected';
});
socket.on('reconnecting', function () {
    $('#socketio-info').text('reconnecting');
    currState = 'reconnecting';
});
socket.on('error', function (err) {
    $('#socketio-info').text(err);
});

socket.on('replaced', function () {
    // XXX: how to minimize the chance we'll further communciate with the
    // server or reconnect to it?  socket.reconnection()?

    // We'd like to window.close() here but can't (not allowed from scripts).
    // Alert is the next best thing.
    alert('Debugger connection replaced by a new one, do you have multiple tabs open? If so, please close this tab.');
});

socket.on('keepalive', function (msg) {
    // Not really interesting in the UI
    // $('#server-info').text(new Date() + ': ' + JSON.stringify(msg));
});

socket.on('basic-info', function (msg) {
    $('#duk-version').text(String(msg.duk_version));
    $('#duk-git-describe').text(String(msg.duk_git_describe));
    $('#target-info').text(String(msg.target_info));
    $('#endianness').text(String(msg.endianness));
});

socket.on('exec-status', function (msg) {
    // Not 100% reliable if callstack has several functions of the same name
    if (bytecodeDialogOpen && (currFileName != msg.fileName || currFuncName != msg.funcName)) {
        socket.emit('get-bytecode', {});
    }

    currFileName = msg.fileName;
    currFuncName = msg.funcName;
    currLine = msg.line;
    currPc = msg.pc;
    currState = msg.state;
    currAttached = msg.attached;

    // Duktape now restricts execution status updates quite effectively so
    // there's no need to rate limit UI updates now.

    doUiUpdate();

    prevState = currState;
    prevAttached = currAttached;
});

// Update the "console" output based on lines sent by the server.  The server
// rate limits these updates to keep the browser load under control.  Even
// better would be for the client to pull this (and other stuff) on its own.
socket.on('output-lines', function (msg) {
    var elem = $('#output');
    var i, n, ent;

    elem.empty();
    for (i = 0, n = msg.length; i < n; i++) {
        ent = msg[i];
        if (ent.type === 'print') {
            elem.append($('<div></div>').text(ent.message));
        } else if (ent.type === 'alert') {
            elem.append($('<div class="alert"></div>').text(ent.message));
        } else if (ent.type === 'log') {
            elem.append($('<div class="log loglevel' + ent.level + '"></div>').text(ent.message));
        } else if (ent.type === 'debugger-info') {
            elem.append($('<div class="debugger-info"><div>').text(ent.message));
        } else if (ent.type === 'debugger-debug') {
            elem.append($('<div class="debugger-debug"><div>').text(ent.message));
        } else {
            elem.append($('<div></div>').text(ent.message));
        }
    }

    // http://stackoverflow.com/questions/14918787/jquery-scroll-to-bottom-of-div-even-after-it-updates
    // Stop queued animations so that we always scroll quickly to bottom
    $('#output').stop(true);
    $('#output').animate({ scrollTop: $('#output')[0].scrollHeight}, 1000);
});

socket.on('callstack', function (msg) {
    var elem = $('#callstack');
    var s1, s2, div;

    currCallstack = msg.callstack;

    elem.empty();
    msg.callstack.forEach(function (e) {
        s1 = $('<a class="rest"></a>').text(e.fileName + ':' + e.lineNumber + ' (pc ' + e.pc + ')');  // float
        s1.on('click', function () {
            activeFileName = e.fileName;
            activeLine = e.lineNumber || 1;
            activeHighlight = activeLine;
            requestSourceRefetch();
        });
        s2 = $('<span class="func"></span>').text(e.funcName + '()');
        div = $('<div></div>');
        div.append(s1);
        div.append(s2);
        elem.append(div);
    });
});

socket.on('locals', function (msg) {
    var elem = $('#locals');
    var s1, s2, div;
    var i, n, e;

    currLocals = msg.locals;

    elem.empty();
    for (i = 0, n = msg.locals.length; i < n; i++) {
        e = msg.locals[i];
        s1 = $('<span class="value"></span>').text(e.value);  // float
        s2 = $('<span class="key"></span>').text(e.key);
        div = $('<div></div>');
        div.append(s1);
        div.append(s2);
        elem.append(div);
    }
});

socket.on('debug-stats', function (msg) {
    $('#debug-rx-bytes').text(formatBytes(msg.rxBytes));
    $('#debug-rx-dvalues').text(msg.rxDvalues);
    $('#debug-rx-messages').text(msg.rxMessages);
    $('#debug-rx-kbrate').text((msg.rxBytesPerSec / 1024).toFixed(2));
    $('#debug-tx-bytes').text(formatBytes(msg.txBytes));
    $('#debug-tx-dvalues').text(msg.txDvalues);
    $('#debug-tx-messages').text(msg.txMessages);
    $('#debug-tx-kbrate').text((msg.txBytesPerSec / 1024).toFixed(2));
});

socket.on('breakpoints', function (msg) {
    var elem = $('#breakpoints');
    var div;
    var sub;

    currBreakpoints = msg.breakpoints;

    elem.empty();

    // First line is special
    div = $('<div></div>');
    sub = $('<button id="delete-all-breakpoints-button"></button>').text('Delete all breakpoints');
    sub.on('click', function () {
        socket.emit('delete-all-breakpoints');
    });
    div.append(sub);
    sub = $('<input id="add-breakpoint-file"></input>').val('file.js');
    div.append(sub);
    sub = $('<span></span>').text(':');
    div.append(sub);
    sub = $('<input id="add-breakpoint-line"></input>').val('123');
    div.append(sub);
    sub = $('<button id="add-breakpoint-button"></button>').text('Add breakpoint');
    sub.on('click', function () {
        socket.emit('add-breakpoint', {
            fileName: $('#add-breakpoint-file').val(),
            lineNumber: Number($('#add-breakpoint-line').val())
        });
    });
    div.append(sub);
    sub = $('<span id="breakpoint-hint"></span>').text('or dblclick source');
    div.append(sub);
    elem.append(div);

    // Active breakpoints follow
    msg.breakpoints.forEach(function (bp) {
        var div;
        var sub;

        div = $('<div class="breakpoint-line"></div>');
        sub = $('<button class="delete-breakpoint-button"></button>').text('Delete');
        sub.on('click', function () {
            socket.emit('delete-breakpoint', {
                fileName: bp.fileName,
                lineNumber: bp.lineNumber
            });
        });
        div.append(sub);
        sub = $('<a></a>').text((bp.fileName || '?') + ':' + (bp.lineNumber || 0));
        sub.on('click', function () {
            activeFileName = bp.fileName || '';
            activeLine = bp.lineNumber || 1;
            activeHighlight = activeLine;
            requestSourceRefetch();
        });
        div.append(sub);
        elem.append(div);
    });

    forceButtonUpdate = true;
    doUiUpdate();
});

socket.on('eval-result', function (msg) {
    $('#eval-output').text((msg.error ? 'ERROR: ' : '') + msg.result);

    // Remove eval button "pulsating" glow when we get a result
    $('#eval-button').removeClass('pending');
});

socket.on('getvar-result', function (msg) {
    $('#var-output').text(msg.found ? msg.result : 'NOTFOUND');
});

socket.on('bytecode', function (msg) {
    var elem, div;
    var div;

    elem = $('#bytecode-preformatted');
    elem.empty();

    msg.preformatted.split('\n').forEach(function (line, idx) {
        div = $('<div></div>');
        div.text(line);
        elem.append(div);
    });

    bytecodeIdxHighlight = null;
    bytecodeIdxInstr = msg.idxPreformattedInstructions;
});

$('#stepinto-button').click(function () {
    socket.emit('stepinto', {});
});

$('#stepover-button').click(function () {
    socket.emit('stepover', {});
});

$('#stepout-button').click(function () {
    socket.emit('stepout', {});
});

$('#pause-button').click(function () {
    socket.emit('pause', {});

    // Pause may take seconds to complete so indicate it is pending.
    $('#pause-button').addClass('pending');
});

$('#resume-button').click(function () {
    socket.emit('resume', {});
});

$('#attach-button').click(function () {
    socket.emit('attach', {});
});

$('#detach-button').click(function () {
    socket.emit('detach', {});
});

$('#about-button').click(function () {
    $('#about-dialog').dialog('open');
});

$('#show-bytecode-button').click(function () {
    bytecodeDialogOpen = true;
    $('#bytecode-dialog').dialog('open');

    elem = $('#bytecode-preformatted');
    elem.empty().text('Loading bytecode...');

    socket.emit('get-bytecode', {});
});

function submitEval() {
    socket.emit('eval', { input: $('#eval-input').val() });

    // Eval may take seconds to complete so indicate it is pending.
    $('#eval-button').addClass('pending');
}

$('#eval-button').click(function () {
    submitEval();
    $('#eval-input').val('');
});

$('#getvar-button').click(function () {
    socket.emit('getvar', { varname: $('#varname-input').val() });
});

$('#putvar-button').click(function () {
    // The variable value is parsed as JSON right now, but it'd be better to
    // also be able to parse buffer values etc.
    var val = JSON.parse($('#varvalue-input').val());
    socket.emit('putvar', { varname: $('#varname-input').val(), varvalue: val });
});

$('#source-code').dblclick(function (event) {
    var target = event.target;
    var elems = $('#source-code div');
    var i, n;
    var line = 0;

    // XXX: any faster way; elems doesn't have e.g. indexOf()
    for (i = 0, n = elems.length; i < n; i++) {
        if (target === elems[i]) {
            line = i + 1;
        }
    }

    socket.emit('toggle-breakpoint', {
        fileName: loadedFileName,
        lineNumber: line
    });
});

function setSourceText(data) {
    var elem, div;

    elem = $('#source-code');
    elem.empty();
    data.split('\n').forEach(function (line) {
        div = $('<div></div>');
        div.text(line);
        elem.append(div);
    });

    sourceEditedLines = [];
}

function setSourceSelect(fileName) {
    var elem;
    var i, n, t;

    if (fileName == null) {
        $('#source-select').val('__none__');
        return;
    }

    elem = $('#source-select option');
    for (i = 0, n = elem.length; i < n; i++) {
        // Exact match is required.
        t = $(elem[i]).val();
        if (t === fileName) {
            $('#source-select').val(t);
            return;
        }
    }
}

/*
 *  AJAX request handling to fetch source files
 */

function requestSourceRefetch() {
    // If previous update is pending, abort and start a new one.
    if (sourceFetchXhr) {
        sourceFetchXhr.abort();
        sourceFetchXhr = null;
    }

    // Make copies of the requested file/line so that we have the proper
    // values in case they've changed.
    var fileName = activeFileName;
    var lineNumber = activeLine;

    // AJAX request for the source.
    sourceFetchXhr = $.ajax({
        type: 'POST',
        url: '/source',
        data: JSON.stringify({ fileName: fileName }),
        contentType: 'application/json',
        success: function (data, status, jqxhr) {
            var elem;

            sourceFetchXhr = null;

            loadedFileName = fileName;
            loadedLineCount = data.split('\n').length;  // XXX: ignore issue with last empty line for now
            loadedFileExecuting = (loadedFileName === currFileName);
            setSourceText(data);
            setSourceSelect(fileName);
            loadedLinePending = activeLine || 1;
            highlightLine = activeHighlight;  // may be null
            activeLine = null;
            activeHighlight = null;
            doSourceUpdate();

            // XXX: hacky transition, make source change visible
            $('#source-pre').fadeTo('fast', 0.25, function () {
                $('#source-pre').fadeTo('fast', 1.0);
            });
        },
        error: function (jqxhr, status, err) {
            // Not worth alerting about because source fetch errors happen
            // all the time, e.g. for dynamically evaluated code.

            sourceFetchXhr = null;

            // XXX: prevent retry of no-such-file by negative caching?
            loadedFileName = fileName;
            loadedLineCount = 1;
            loadedFileExecuting = false;
            setSourceText('// Cannot load source file: ' + fileName);
            setSourceSelect(null);
            loadedLinePending = 1;
            activeLine = null;
            activeHighlight = null;
            doSourceUpdate();

            // XXX: error transition here
            $('#source-pre').fadeTo('fast', 0.25, function () {
                $('#source-pre').fadeTo('fast', 1.0);
            });
        },
        dataType: 'text'
    });
}

/*
 *  AJAX request for fetching the source list
 */

function fetchSourceList() {
    $.ajax({
        type: 'POST',
        url: '/sourceList',
        data: JSON.stringify({}),
        contentType: 'application/json',
        success: function (data, status, jqxhr) {
            var elem = $('#source-select');

            data = JSON.parse(data);

            elem.empty();
            var opt = $('<option></option>').attr({ 'value': '__none__' }).text('No source file selected');
            elem.append(opt);
            data.forEach(function (ent) {
                var opt = $('<option></option>').attr({ 'value': ent }).text(ent);
                elem.append(opt);
            });
            elem.change(function () {
                activeFileName = elem.val();
                activeLine = 1;
                requestSourceRefetch();
            });
        },
        error: function (jqxhr, status, err) {
            // This is worth alerting about as the UI is somewhat unusable
            // if we don't get a source list.

            alert('Failed to load source list: ' + err);
        },
        dataType: 'text'
    });
}

/*
 *  Initialization
 */

$(document).ready(function () {
    var showAbout = true;

    // About dialog, shown automatically on first startup.
    $('#about-dialog').dialog({
        autoOpen: false,
        hide: 'fade',  // puff
        show: 'fade',  // slide, puff
        width: 500,
        height: 300
    });

    // Bytecode dialog
    $('#bytecode-dialog').dialog({
        autoOpen: false,
        hide: 'fade',  // puff
        show: 'fade',  // slide, puff
        width: 700,
        height: 600,
        close: function () {
            bytecodeDialogOpen = false;
            bytecodeIdxHighlight = null;
            bytecodeIdxInstr = 0;
        }
    });

    // http://diveintohtml5.info/storage.html
    if (typeof localStorage !== 'undefined') {
        if (localStorage.getItem('about-shown')) {
            showAbout = false;
        } else {
            localStorage.setItem('about-shown', 'yes');
        }
    }
    if (showAbout) {
        $('#about-dialog').dialog('open');
    }

    // onclick handler for exec status text
    function loadCurrFunc() {
        activeFileName = currFileName;
        activeLine = currLine;
        requestSourceRefetch();
    }
    $('#exec-other').on('click', loadCurrFunc);

    // Enter handling for eval input
    // https://forum.jquery.com/topic/bind-html-input-to-enter-key-keypress
    $('#eval-input').keypress(function (event) {
        if (event.keyCode == 13) {
            submitEval();
            $('#eval-input').val('');
        }
    });

    // Eval watch handling
    $('#eval-watch').change(function () {
        // nop
    });

    // Function keys
    $('body').keydown(function(e){
        //alert("keydown: "+e.which);
        switch (e.which) {
        case 116:  // F5: step into
            socket.emit('stepinto', {});
            e.preventDefault();
            return;
        case 117:  // F6: step over
            socket.emit('stepover', {});
            e.preventDefault();
            return;
        case 118:  // F7: step out (= step return)
            socket.emit('stepout', {});
            e.preventDefault();
            return;
        case 119:  // F8: resume
            socket.emit('resume', {});
            e.preventDefault();
            return;
        }
    });

    forceButtonUpdate = true;
    doUiUpdate();
});
