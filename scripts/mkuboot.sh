#!/bin/bash

#
# Build U-Boot image when `mkimage' tool is available.
#

MKIMAGE=$(type -path "${CROSS_COMPILE}mkimage")

if [ -z "${MKIMAGE}" ]; then
	MKIMAGE=$(type -path mkimage)
	if [ -z "${MKIMAGE}" ]; then
		# Doesn't exist
		echo '"mkimage" command not found - U-Boot images will not be built' >&2
		exit 1;
	fi
fi
LOAD_ADDRESS=80010000
OBVER=`${MKIMAGE} "$@"`
echo "$OBVER"
if [[ "$OBVER" != *"$LOAD_ADDRESS"* ]]; then
    echo "Load address error, please upgrade \"mkimage\" command and recompile"
    exit 1;
fi

