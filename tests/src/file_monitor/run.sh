#!/bin/sh

basedir=`dirname $0`

cd "$basedir"

rm -r target

./a.out &

sleep 1 && mkdir -p target/sub
sleep 1 && echo "11" > target/sub/file1
sleep 1 && echo "12" > target/sub/file1
sleep 1 && echo "21" > target/sub/file2
sleep 1 && echo "22" > target/sub/file2
sleep 1 && echo "13" > target/sub/file1
sleep 1 && rm target/sub/file2
sleep 1 && rm -r target
sleep 1 && mkdir -p target/sub
sleep 1 && echo "14" > target/sub/file1
sleep 1 && echo "15" > target/sub/file1.new && mv target/sub/file1.new target/sub/file1

sleep 1 && echo "end" > target/sub/file1
sleep 1 && echo && echo
