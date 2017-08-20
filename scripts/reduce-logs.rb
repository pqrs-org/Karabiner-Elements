#!/usr/bin/env ruby

command=ARGV[0]

if command.nil? then
  print "Usage: #{$0} command\n"
  exit 1
end

count = 0
full_output = ''
open("| #{command} 2>&1") do |f|
  while l = f.gets
    full_output += l

    print '.'
    count += 1
    if count >= 70 then
      print "\n"
      count = 0
    end
  end
end
print "\n"

if $?.exitstatus != 0 then
  print full_output
  print "\n"
  exit 99
end

exit 0
