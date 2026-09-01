#!/bin/bash

if test "$#" -ne 1; then
	echo "Usage: > $0 shipped object file"
	exit
fi

VERINFO=`$OBJDUMP -h $1 | grep "\.version" | sed 's/.*\.version\.\([^\ ]*\).*/\1/g'`
if [ -z "${VERINFO:-}" ]; then
		echo $1
	else
		echo -e $1 ["$VERINFO"]
	fi