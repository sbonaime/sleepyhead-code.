#!/bin/bash
MY_PATH="`dirname \"$0\"`"
if [ ! -f "$MY_PATH/ReleaseMode" ]
then
	echo "Skipping build number update"
	exit;
fi
echo "Updating build number"
number=`cat $MY_PATH/build_number`
let number++
echo "$number" > $MY_PATH/build_number
echo "const int build_number = ""$number;" | tee $MY_PATH/../build_number.h
