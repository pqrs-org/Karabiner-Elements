/*
 *  Object.prototype.__defineGetter__ polyfill
 */

(function () {
    if (typeof Object.prototype.__defineGetter__ === 'undefined') {
        var DP = Object.defineProperty;
        DP(Object.prototype, '__defineGetter__', {
            value: function (n, f) {
                DP(this, n, { enumerable: true, configurable: true, get: f });
            }, writable: true, enumerable: false, configurable: true
        });
    }
})();
