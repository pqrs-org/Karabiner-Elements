// process.js
function processLine(line) {
    return line.trim()
        .replace(/[<>&"'\u0000-\u001F\u007E-\uFFFF]/g, function(x) {
            // escape HTML characters
            return '&#' + x.charCodeAt(0) + ';'
         })
        .replace(/\*(.*?)\*/g, function(x, m) {
            // automatically bold text between stars
            return '<b>' + m + '</b>';
         });
}
