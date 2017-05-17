#!/bin/bash

TIMER=2

while true
do
	sleep $TIMER
	now=$(date "+%d_%m_%H_%M_%S")
	msg='"Autocommit at '$now\"
	echo $msg
	git add --all
	git commit -m "$msg"
	git push
done
