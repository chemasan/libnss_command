#!/usr/bin/env bash

# Copyright (c) 2017 Jose Manuel Sanchez Madrid.
# This file is licensed under MIT license. See file LICENSE for details.

function main()
{
	if [ $# -lt 1 ]
	then
		return 3
	fi
	local addr="$1"
	if [ -z "${addr}" ]
	then
		return 3
	fi
	case "${addr}" in
		(127.0.0.2)
			echo "name: myhost.local."
			echo "alias: myhost"
			echo "alias: myalias.local."
			echo "ip4: 127.0.0.1"
			echo "ip4: 127.0.0.2"
			return 0
			;;
		(*)
			return 1
	esac
}

main "$@"
exit $?
