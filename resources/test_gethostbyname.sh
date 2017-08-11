#!/usr/bin/env bash

# Copyright (c) 2017 Jose Manuel Sanchez Madrid.
# This file is licensed under MIT license. See file LICENSE for details.

function main()
{
	if [ $# -lt 1 ]
	then
		return 3
	fi
	local name="$1"
	if [ -z "${name}" ]
	then
		return 3
	fi
	case "${name}" in
		(myhost|myhost.local|myhost.local.|myalias.local|myalias.local.)
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
