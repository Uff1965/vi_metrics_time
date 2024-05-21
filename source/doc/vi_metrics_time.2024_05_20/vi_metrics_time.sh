#!/bin/bash

echo START: $(date '+%Y-%m-%d')
echo ""

program_name=vi_metrics_time

cnt=1
if [ "$1" ]
then
	cnt=$1
fi

par=${@:2}
echo Parameters: "'"$par"'"
echo ""

#Push a HOSTNAME into the child process
export HOSTNAME=$(hostname)

for i in $(seq 1 $cnt)
do
	echo $(date '+%H:%M:%S'): Started measurement $i of $cnt...
	sudo -E nice -n -5 ./$program_name $par >$program_name.$HOSTNAME.$i.txt
#	sleep 1
	echo done.
	echo ""
done

echo FINISH: $(date '+%Y-%m-%d %H:%M:%S')
