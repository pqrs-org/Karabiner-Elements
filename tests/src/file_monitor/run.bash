#!/bin/bash

basedir=$(dirname $0)

cd "$basedir"

rm -rf target

./build/test &

sleep 1 && mkdir -p target/sub
# generic file modification
sleep 1 && echo "11" >target/sub/file1
sleep 1 && echo "12" >target/sub/file1
sleep 1 && echo "21" >target/sub/file2
sleep 1 && echo "22" >target/sub/file2
sleep 1 && echo "13" >target/sub/file1
# file removal
sleep 1 && rm target/sub/file2
# directory removal
sleep 1 && rm -r target
sleep 1 && mkdir -p target/sub
sleep 1 && echo "14" >target/sub/file1
# move file
sleep 1 && echo "15" >target/sub/file1.new && mv target/sub/file1.new target/sub/file1
# move directory
sleep 1 && rm -r target
sleep 1 && mkdir -p target.new/sub && echo "16" >>target.new/sub/file1 && mv target.new target

# end
sleep 1 && echo "end" >target/sub/file1
sleep 1 && echo && echo
