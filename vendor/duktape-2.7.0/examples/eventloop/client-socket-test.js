var HOST = 'localhost';
var PORT = 80;

EventLoop.connect(HOST, PORT, function (fd) {
    print('connected to ' + HOST + ':' + PORT + ', fd', fd);
    EventLoop.setReader(fd, function (fd, data) {
        print('read from fd', fd);
        print(new TextDecoder().decode(data));
        // Read until completion, socket is closed by server.
    });
    EventLoop.write(fd, "GET / HTTP/1.1\r\n" +
                        "Host: " + HOST + "\r\n" +
                        "User-Agent: client-socket-test.js\r\n" +
                        "Connection: close\r\n" +
                        "\r\n");
});
