#!/usr/bin/ruby

require 'json'

print "Checking types.hpp...\n"

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

# check IOHIDUsageTables.h

open('vendor/IOHIDUsageTables.h') do |f|
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
  print "\n"
end

# check simple_modifications.json

print "check simple_modifications.json"

def check_simple_modifications(simple_modifications, key_code_map)
  simple_modifications.each do |value|
    unless value["children"].nil? then
      check_simple_modifications(value["children"], key_code_map)
    end
    unless value["name"].nil? then
      print '.'

      v = key_code_map[value["name"]]

      if v.nil? then
        print "Error: Unknown name `#{value["name"]}`\n"
        exit 1
      end

      if v["hid_system_key"].nil? and v["hid_system_aux_control_button"].nil? then
        # this key should be not_to
        if value["not_to"].nil? then
          if value["name"] != "vk_none" then
            print "Error: `#{value["name"]}` should be not_to\n"
            exit 1
          end
        end
      else
        # this key should not be not_to
        unless value["not_to"].nil? then
          print "Error: `#{value["name"]}` should not be not_to\n"
          exit 1
        end
      end
    end
  end
end

open('../../src/apps/PreferencesWindow/PreferencesWindow/Resources/simple_modifications.json') do |f|
  simple_modifications = JSON.parse(f.read)
  check_simple_modifications(simple_modifications, key_code_map)
  print "\n"
end
