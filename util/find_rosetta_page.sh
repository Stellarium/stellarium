#!/bin/bash

#
# (C) Stellarium Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 
# 02110-1335, USA.


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

