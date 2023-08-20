var HOST = 'localhost'
var PORT = 12345;
var EXIT_TIMEOUT = 300e3;

print('automatic exit after ' + (EXIT_TIMEOUT / 1e3) + ' seconds');
setTimeout(function () {
    print('exit timer');
    EventLoop.requestExit();
}, EXIT_TIMEOUT);

print('listen on ' + HOST + ':' + PORT);
EventLoop.server(HOST, PORT, function (fd, addr, port) {
    print('new connection on fd ' + fd + ' from ' + addr + ':' + port);
    EventLoop.setReader(fd, function (fd, data) {
        var b, i, n, x;

        // Handle socket data carefully: if you convert it to a string,
        // it may not be valid UTF-8 etc.  Here we operate on the data
        // directly in the buffer.

        b = data.valueOf();  // ensure we get a plain buffer
        n = b.length;
        for (i = 0; i < n; i++) {
            x = b[i];
            if (x >= 0x61 && x <= 0x7a) {
                b[i] = x - 0x20;  // uppercase
            }
        }

        print('read data on fd ' + fd + ', length ' + data.length);
        EventLoop.write(fd, data);
    });
});
