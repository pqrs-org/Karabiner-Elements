#!/usr/bin/ruby

bomfilepath = ARGV[0]
topdir = ARGV[1]

if bomfilepath.nil? or (not FileTest.file?(bomfilepath)) or
    topdir.nil? or (not FileTest.directory?(topdir)) then
  $stderr.print "Usage: #{$0} Archive.bom topdir\n"
  exit 1
end

open("| lsbom #{bomfilepath}") do |lsbom|
  open("#{bomfilepath}.filelist", "w") do |outfile|
    while l = lsbom.gets
      a = l.chomp.split(/\t/)
      (filepath, mode, owner) = a

      stat = nil
      if /\/\._/ =~ filepath then
        # It's a resource fork. (For example, ._document.wflow).
        # Use original file stat.
        stat = File.lstat(topdir + filepath.gsub(/\/\._/, '/'))
      else
        stat = File.lstat(topdir + filepath)
      end
      newmode = sprintf('%o', stat.mode)

      if newmode != mode then
        print "Change Mode: #{mode} -> #{newmode} #{filepath}\n"
        a[1] = newmode
      end

      newowner = '0/0'
      if newowner != owner then
        print "Change Owner: #{owner} -> #{newowner} #{filepath}\n"
        a[2] = newowner
      end

      outfile.print a.join("\t") + "\n"
    end
  end
end

system("mkbom -i #{bomfilepath}.filelist #{bomfilepath}.new")
system("cp #{bomfilepath}.new #{bomfilepath}")
File.unlink("#{bomfilepath}.filelist")
File.unlink("#{bomfilepath}.new")
