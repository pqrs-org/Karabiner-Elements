mandel = (x0, y0, x1, y1, w, h, maxiter) ->
  [dx, dy] = [(x1 - x0) / w, (y1 - y0) / h]
  res = []

  y = y0
  for yc in [0..h-1]
    x = x0
    for xc in [0..w-1]
      [xx, yy] = [x, y]
      c = '*'
      for i in [0..maxiter-1]
        # (xx+i*yy)^2 + (x+i*y) = xx^2 + i*2*xx*yy - yy^2 + x + i*y
        # = (xx^2 - yy^2 + x) + i*(2*xx*yy + y)
        [xx2, yy2] = [xx*xx, yy*yy]
        if xx2 + yy2 >= 4.0
          c = '.'
          break
        [xx, yy] = [xx2 - yy2 + x, 2*xx*yy + y]
      res.push(c)
      x += dx
    res.push('\n')
    y += dy

  print(res.join(''))
  return

mandel(-2, 2, 2, -2, 200, 100, 1000)

