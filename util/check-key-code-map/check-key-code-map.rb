#!/usr/bin/ruby

require 'json'

print "Checking userspace_types.hpp...\n"

class String
  def to_snake()
    self
      .gsub(/([A-Z]+)([A-Z][a-z])/, '\1_\2')
      .gsub(/([a-z\d])([A-Z])/, '\1_\2')
      .tr("-", "_")
      .downcase
  end
end

key_code_map = nil
open('| ./dump_key_code_map/build/Release/dump_key_code_map') do |f|
  key_code_map = JSON.parse(f.read)
end

open('IOHIDUsageTables.h') do |f|
  is_target = false
  count = 0
  while l = f.gets
    l.encode!('UTF-8', 'binary', invalid: :replace, undef: :replace, replace: '')

    if /^\s+\/\*/ =~ l then
      # skip comment
      next
    end

    case l
    when / kHIDUsage_KeyboardErrorUndefined /
      is_target = true
    when / kHIDUsage_Keyboard_Reserved /
      is_target = false
    else
      if is_target then
        if /^\s+(.+?)=/ =~ l then
          usage_name = $1.strip

          if /^kHIDUsage_Keyboard(.+)/ =~ usage_name then
            key_code_name = $1.to_snake
          elsif /^kHIDUsage_Keypad(.+)/ =~ usage_name then
            key_code_name = 'keypad_' + $1.to_snake
          else
            print "Error: Unknown usage_name:\n"
            print "  " + usage_name + "\n"
            exit 1
          end

          print '.'
          count += 1
          if count > 60 then
            count = 0
            print "\n"
          end
          if key_code_map[key_code_name].nil? then
            print "Error: key_code_name is not in key_code_map:\n"
            print "  " + key_code_name + "\n"
            exit 1
          end
        else
          print "Error: Invalid line in IOHIDUsageTables:\n"
          print "  " + l + "\n"
          exit 1
        end
      end
    end
  end
  print "\n"
end
