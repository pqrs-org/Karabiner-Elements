#!/usr/bin/ruby

count = 0
while l = gets
  print '.'
  count += 1
  if count >= 70 then
    print "\n"
    count = 0
  end
end
print "\n"
