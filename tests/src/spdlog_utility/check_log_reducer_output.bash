#!/bin/sh

cd $(dirname $0)

./build/karabiner_test >log

[ $(grep test1 log | grep -c info) != '1' ] && exit 1
[ $(grep test1 log | grep -c warn) != '1' ] && exit 1
[ $(grep test1 log | grep -c error) != '1' ] && exit 1

[ $(grep -c test2 log) != '3' ] && exit 1

[ $(grep -c test3 log) != '2' ] && exit 1

rm -f log

exit 0
