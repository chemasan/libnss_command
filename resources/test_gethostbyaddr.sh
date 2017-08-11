#!/usr/bin/env bash

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
