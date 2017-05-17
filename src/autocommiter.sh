#!/bin/bash

TIMER=600 # 30 minutes

while true
do
	now=$(date "+%d_%m_%H_%M_%S")
	msg='"Autocommit at '$now\"
	echo $msg
	git add --all
	git commit -m "$msg"
	git push
	sleep $TIMER
done
