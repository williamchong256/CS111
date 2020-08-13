#!/bin/bash

#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665


#{ sleep 2; echo "START"; sleep 2; echo "STOP"; sleep 2; echo "PERIOD=2"; sleep 2; echo "SCALE=F"; sleep 2; echo "SCALE=C"; sleep 2; echo "OFF"; } | ./lab4b --log=log
./lab4b --period=3 --scale=F --log=log > STDOUT 2> STDERR <<-EOF

SCALE=C
PERIOD=2
STOP
SCALE=F
START
LOG
OFF

EOF

for t in "START" "STOP" "PERIOD=2" "SCALE=F" "SCALE=C" "OFF" "SHUTDOWN"
	do
		grep "$t" log > /dev/null
		if [ $? -ne 0 ]
		then
			echo "FAILED TO LOG $t COMMAND"
		else
			echo "SUCCESSFULLY LOGGED $t COMMAND"
		fi
	done

cat log
rm -f log STDOUT STDERR
