#!/bin/bash

echo "Countdown 3"
sleep 1
echo "Countdown 2"
sleep 1
echo "Countdown 1"
sleep 1
echo "Start"

for i in $(seq 1 10); do
	./build/a.out >/dev/null &
done

./build/a.out

sleep 1
echo "Finished"
