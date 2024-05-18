#!/bin/bash
# Comment )

echo START: $(date)

program_name=vi_metrics_time

number_of_times=$1
if [ -z "$1" ]
then
	number_of_times=$1
fi

for i in $(seq 1 $number_of_times)
do
	echo $(date)
	echo Started measurement $i of $number_of_times...
	./$program_name >$program_name.$i.txt
	echo done.
done

echo FINISH: $(date)
