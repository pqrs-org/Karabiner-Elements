#!/usr/bin/ruby

require 'json'

$configuration = JSON.parse($stdin.read)

def check_value(name, value, configuration = nil)
  return if ARGV.include?(name)

  root = configuration.nil?

  if configuration.nil? then
    print "  check #{name}\n"
    configuration = $configuration
  end

  exists = false
  configuration.each do |k,v|
    if v.kind_of?(Enumerable) then
      if check_value(name, value, v) then
        exists = true
      end
    else
      if name == k then
        exists = true
        if value.nil? then
          print "[ERROR] Appear name: #{name}\n"
          exit 1
        elsif value != v then
          print "[ERROR] Invalid value: #{name} (#{v} != #{value})\n"
          exit 1
        end
      end
    end
  end

  if root and (not value.nil?) and (not exists) then
    print "[ERROR] No setting: #{name}\n"
    exit 1
  end

  exists
end

check_value('objectVersion', '47')
check_value('GCC_TREAT_WARNINGS_AS_ERRORS', 'YES')
check_value('GCC_WARN_64_TO_32_BIT_CONVERSION', 'YES')
check_value('GCC_WARN_ABOUT_MISSING_NEWLINE', 'YES')
check_value('GCC_WARN_ABOUT_MISSING_PROTOTYPES', 'YES')
check_value('GCC_WARN_ABOUT_RETURN_TYPE', 'YES')
check_value('GCC_WARN_CHECK_SWITCH_STATEMENTS', 'YES')
check_value('GCC_WARN_NON_VIRTUAL_DESTRUCTOR', 'YES')
check_value('GCC_WARN_MISSING_PARENTHESES', 'YES')
check_value('GCC_WARN_SHADOW', 'YES')
check_value('GCC_WARN_SIGN_COMPARE', 'YES')
check_value('GCC_WARN_UNDECLARED_SELECTOR', 'YES')
check_value('GCC_WARN_UNINITIALIZED_AUTOS', 'YES')
check_value('GCC_WARN_UNUSED_FUNCTION', 'YES')
check_value('GCC_WARN_UNUSED_LABEL', 'YES')
check_value('GCC_WARN_UNUSED_VALUE', 'YES')
check_value('GCC_WARN_UNUSED_VARIABLE', 'YES')
check_value('CLANG_ENABLE_OBJC_ARC', 'YES')
check_value('MACOSX_DEPLOYMENT_TARGET', '10.10')
check_value('RUN_CLANG_STATIC_ANALYZER', 'YES')
check_value('SDKROOT', 'macosx')

check_value('GCC_WARN_PROTOTYPE_CONVERSION', nil)
check_value('ARCHS', nil)
