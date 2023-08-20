/*
 *  A few basic tests
 */

var count = 0;
var intervalId;

setTimeout(function (x) { print('timer 1', x); }, 1234, 'foo');
setTimeout('print("timer 2");', 4321);
setTimeout(function () { print('timer 3'); }, 2345);
intervalId = setInterval(function (x, y) {
    print('interval', ++count, x, y);
    if (count >= 10) {
        clearInterval(intervalId);
    }
}, 400, 'foo', 'bar');
