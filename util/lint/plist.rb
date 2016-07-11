#!/usr/bin/ruby

hash = {}
key = nil
xml = $stdin.read
xml.scan(/<(.+?)>([^<]*)/m).each do |matches|
  case matches[0]
  when 'key'
    key = matches[1]
  when 'string'
    unless key.nil? then
      hash[key] = matches[1]
    end
  end
end

unless /^org\.pqrs\./ =~ hash['CFBundleIdentifier'] then
  print "CFBundleIdentifier may be invalid. '#{hash['CFBundleIdentifier']}'\n"
  exit 1
end

iconfile = hash['CFBundleIconFile']
if iconfile.nil? or (not /icns$/ =~ iconfile) then
  unless ARGV.include?('CFBundleIconFile')
    print "CFBundleIconFile is invalid. #{iconfile}\n"
    exit 1
  end
end
