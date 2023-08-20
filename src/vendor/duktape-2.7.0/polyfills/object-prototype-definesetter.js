/*
 *  Object.prototype.__defineSetter__ polyfill
 */

(function () {
    if (typeof Object.prototype.__defineSetter__ === 'undefined') {
        var DP = Object.defineProperty;
        DP(Object.prototype, '__defineSetter__', {
            value: function (n, f) {
                DP(this, n, { enumerable: true, configurable: true, set: f });
            }, writable: true, enumerable: false, configurable: true
        });
    }
})();
