/*
 *  C eventloop example (c_eventloop.c).
 *
 *  ECMAScript code to initialize the exposed API (setTimeout() etc) when
 *  using the C eventloop.
 *
 *  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Timers
 */

/*
 *  Timer API
 */

function setTimeout(func, delay) {
    var cb_func;
    var bind_args;
    var timer_id;

    // Delay can be optional at least in some contexts, so tolerate that.
    // https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/setTimeout
    if (typeof delay !== 'number') {
        if (typeof delay === 'undefined') {
            delay = 0;
        } else {
            throw new TypeError('invalid delay');
        }
    }

    if (typeof func === 'string') {
        // Legacy case: callback is a string.
        cb_func = eval.bind(this, func);
    } else if (typeof func !== 'function') {
        throw new TypeError('callback is not a function/string');
    } else if (arguments.length > 2) {
        // Special case: callback arguments are provided.
        bind_args = Array.prototype.slice.call(arguments, 2);  // [ arg1, arg2, ... ]
        bind_args.unshift(this);  // [ global(this), arg1, arg2, ... ]
        cb_func = func.bind.apply(func, bind_args);
    } else {
        // Normal case: callback given as a function without arguments.
        cb_func = func;
    }

    timer_id = EventLoop.createTimer(cb_func, delay, true /*oneshot*/);

    return timer_id;
}

function clearTimeout(timer_id) {
    if (typeof timer_id !== 'number') {
        throw new TypeError('timer ID is not a number');
    }
    var success = EventLoop.deleteTimer(timer_id);  /* retval ignored */
}

function setInterval(func, delay) {
    var cb_func;
    var bind_args;
    var timer_id;

    if (typeof delay !== 'number') {
        if (typeof delay === 'undefined') {
            delay = 0;
        } else {
            throw new TypeError('invalid delay');
        }
    }

    if (typeof func === 'string') {
        // Legacy case: callback is a string.
        cb_func = eval.bind(this, func);
    } else if (typeof func !== 'function') {
        throw new TypeError('callback is not a function/string');
    } else if (arguments.length > 2) {
        // Special case: callback arguments are provided.
        bind_args = Array.prototype.slice.call(arguments, 2);  // [ arg1, arg2, ... ]
        bind_args.unshift(this);  // [ global(this), arg1, arg2, ... ]
        cb_func = func.bind.apply(func, bind_args);
    } else {
        // Normal case: callback given as a function without arguments.
        cb_func = func;
    }

    timer_id = EventLoop.createTimer(cb_func, delay, false /*oneshot*/);

    return timer_id;
}

function clearInterval(timer_id) {
    if (typeof timer_id !== 'number') {
        throw new TypeError('timer ID is not a number');
    }
    EventLoop.deleteTimer(timer_id);
}

function requestEventLoopExit() {
    EventLoop.requestExit();
}

/*
 *  Socket handling
 *
 *  Ideally this would be implemented more in C than here for more speed
 *  and smaller footprint: C code would directly maintain the callback state
 *  and such.
 *
 *  Also for more optimal I/O, the buffer churn caused by allocating and
 *  freeing a lot of buffer values could be eliminated by reusing buffers.
 *  Socket reads would then go into a pre-allocated buffer, for instance.
 */

EventLoop.socketListening = {};
EventLoop.socketReading = {};
EventLoop.socketConnecting = {};

EventLoop.fdPollHandler = function(fd, revents) {
    var data;
    var cb;
    var rc;
    var acc_res;

    //print('activity on fd', fd, 'revents', revents);

    if (revents & Poll.POLLIN) {
        cb = this.socketReading[fd];
        if (cb) {
            data = Socket.read(fd);  // no size control now
            //print('READ', Duktape.enc('jx', data));
            if (data.length === 0) {
                this.close(fd);
                return;
            }
            cb(fd, data);
        } else {
            cb = this.socketListening[fd];
            if (cb) {
                acc_res = Socket.accept(fd);
                //print('ACCEPT:', Duktape.enc('jx', acc_res));
                cb(acc_res.fd, acc_res.addr, acc_res.port);
            } else {
                //print('UNKNOWN');
            }
        }
    }

    if (revents & Poll.POLLOUT) {
        // Connected
        cb = this.socketConnecting[fd];
        if (cb) {
            delete this.socketConnecting[fd];
            cb(fd);
        }
    }

    if ((revents & ~(Poll.POLLIN | Poll.POLLOUT)) !== 0) {
        //print('unexpected revents, close fd');
        this.close(fd);
    }
}

EventLoop.server = function(address, port, cb_accepted) {
    var fd = Socket.createServerSocket(address, port);
    this.socketListening[fd] = cb_accepted;
    this.listenFd(fd, Poll.POLLIN);
}

EventLoop.connect = function(address, port, cb_connected) {
    var fd = Socket.connect(address, port);
    this.socketConnecting[fd] = cb_connected;
    this.listenFd(fd, Poll.POLLOUT);
}

EventLoop.close = function(fd) {
    EventLoop.listenFd(fd, 0);
    delete this.socketListening[fd];
    delete this.socketReading[fd];
    delete this.socketConnecting[fd];
    Socket.close(fd);
}

EventLoop.setReader = function(fd, cb_read) {
    this.socketReading[fd] = cb_read;
    this.listenFd(fd, Poll.POLLIN);
}

EventLoop.write = function(fd, data) {
    // This simple example doesn't have support for write blocking / draining
    if (typeof data === 'string') {
        data = new TextEncoder().encode(data);
    }
    var rc = Socket.write(fd, data);
}
