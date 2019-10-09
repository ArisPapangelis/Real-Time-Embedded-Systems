#!/bin/bash
./bin/espx &
pid=$!
sleep 7200
kill $pid
sed "s/_/,/g;" statistics.log > statistics.csv
sed -i '1s/^/sender,receiver,timestamp,msg\n/' statistics.csv