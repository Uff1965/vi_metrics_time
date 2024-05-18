#!/bin/bash

echo START: $(date '+%Y-%m-%d')
echo ""

program_name=vi_metrics_time

cnt=1
if [ "$1" ]
then
	cnt=$1
fi

rep=20
if [ "$2" ]
then
	rep=$2
fi

if [ "$3" ]
then
	filter="-i $3"
fi

par="-r $rep $filter"
echo Parameters: $par
echo ""

#Push a HOSTNAME into the child process
export HOSTNAME=$HOSTNAME

for i in $(seq 1 $cnt)
do
	echo $(date '+%H:%M:%S'): Started measurement $i of $cnt...
	sudo -E nice -n -5 ./$program_name $par >$program_name.$HOSTNAME.$i.txt
#	sleep 1
	echo done.
	echo ""
done

echo FINISH: $(date '+%Y-%m-%d %H:%M:%S')
