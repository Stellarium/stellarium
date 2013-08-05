#!/bin/bash

main () {
	if [ $# -ne 2 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
		echo "Usage:"
		echo "  ${0##*/} languagecode string"
		echo ""
		echo "Searches Rosetta for the page where a particular translation string is located"
		echo "Outputs URLs which contain the string"
		exit
	fi

	lang="$1"

	# first page
	url="https://translations.launchpad.net/stellarium/0.9/+pots/stellarium/$lang/+translate"
	last=$(GET "$url" |grep results |tail -n 1 |awk '{ print $1 }')
	check_url "$url" "$2"
	for ((f=10; f<${last%[0-9]}0; f+=10)); do 
		url="https://translations.launchpad.net/stellarium/0.9/+pots/stellarium/$lang/+translate?start=$f"
		check_url "$url" "$2"
	done

}

check_url () {
	GET "$1" |grep -q "$2" && echo "$1"
}

main "$@"

