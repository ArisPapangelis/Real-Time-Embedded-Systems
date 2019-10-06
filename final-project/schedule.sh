#!/bin/bash

./espx ips.txt messages.txt &
pid=$!
sleep $1
kill $pid
sed "s/_/,/g;" stats.log > stats.csv
sed "/^$/d;" -i stats.csv
sed -i '1s/^/sender,receiver,timestamp,msg\n/' stats.csv