/*
 *  Mandelbrot example:
 *
 *    $ ./duk mandel.js
 *    [...]
 */

function mandel() {
    var w = 76, h = 28, iter = 100;
    var i, j, k, c;
    var x0, y0, xx, yy, xx2, yy2;
    var line;

    for (i = 0; i < h; i++) {
        y0 = (i / h) * 2.5 - 1.25;

        for (j = 0, line = []; j < w; j++) {
            x0 = (j / w) * 3.0 - 2.0;

            for (k = 0, xx = 0, yy = 0, c = '#'; k < iter; k++) {
                /* z -> z^2 + c
                 *   -> (xx+i*yy)^2 + (x0+i*y0)
                 *   -> xx*xx+i*2*xx*yy-yy*yy + x0 + i*y0
                 *   -> (xx*xx - yy*yy + x0) + i*(2*xx*yy + y0)
                 */

                xx2 = xx*xx; yy2 = yy*yy;

                if (xx2 + yy2 < 4.0) {
                    yy = 2*xx*yy + y0;
                    xx = xx2 - yy2 + x0;
                } else {
                    /* xx^2 + yy^2 >= 4.0 */
                    if (k < 3) { c = '.'; }
                    else if (k < 5) { c = ','; }
                    else if (k < 10) { c = '-'; }
                    else { c = '='; }
                    break;
                }
            }

            line.push(c);
        }

        print(line.join(''));
    }
}

try {
    mandel();
} catch (e) {
    print(e.stack || e);
}
