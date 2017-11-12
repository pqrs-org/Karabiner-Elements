#!/bin/bash

cat `dirname $0`/../version | ruby -ne 'print "v%02d%02d%02d" % $_.strip.split(/\./)'
