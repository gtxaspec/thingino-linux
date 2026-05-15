#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

#
# Build U-Boot image when `mkimage' tool is available.
#

MKIMAGE=$(command -v "${CROSS_COMPILE}mkimage")

if [ -z "${MKIMAGE}" ]; then
	MKIMAGE=$(command -v mkimage)
	if [ -z "${MKIMAGE}" ]; then
		# Doesn't exist
		echo '"mkimage" command not found - U-Boot images will not be built' >&2
		exit 1;
	fi
fi

# Call "mkimage" to create U-Boot image
${MKIMAGE} "$@"
