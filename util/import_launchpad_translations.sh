#!/bin/bash
# 
# Import translations from launchpad.  Pass to this script the path of
# one or more tarballs which have been downloaded from launchpad.  
# It will be imported into your source tree, the root of which which 
# should be identified by the STELROOT environment variable.
#
# Example usage:
#   export STELROOT=/home/me/projects/stellarium
#   import_launchpad_translations.sh ~/downloads/launchpad-export.tar.gz
#
# (C) 2008 Matthew Gates
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
# 02111-1307, USA.

main() {
	po_dir="${STELROOT:?You must set STELROOT before running this program}/po"
	tmp_dir="$(mktemp -d)" || errex 3 "could not create temp directory"
	log="$(mktemp)" || errex "could not create log file"

	for tarball in "$@"; do
		detar "$tarball"
	done

	cd "$po_dir" || errex 2 "ERROR - could not change to po dir: $po_dir"
	
	for group in stellarium stellarium-skycultures; do
		process_group "$group"
	done

	# output warnings last, but uniq them first
	sort -u "$log"
	cd
	rm -f "$log"
	rm -rf "$tmp_dir"
}

detar() {
	case "$1" in
	*.tgz|*.tar.gz)
		tar_opt=z
		;;
	*.tar.bz2)
		tar_opt=j
		;;
	*)
		err "cannot work out tarball type, SKIPPING $1"
		return 1
		;;
	esac

	tar ${tar_opt}xf "$1" -C "$tmp_dir"
	if [ $? -ne 0 ]; then
		err "unable to process tarball, SKIPPING $1"
		return 1
	else
		return 0
	fi
}

process_group() {
	group_dir="$tmp_dir/po/$1"
	if [[ ! -d "$group_dir" ]]; then
		err "process_group: couldn't find group dir $group_dir" 
		return
	fi

	if [[ ! -d "$1" ]]; then
		err "destination translation directory \"$po_dir/$1\" does not exist, SKIPPING group $1"
		return
	fi

	declare -a processed_codes
	echo -n "processing directory \$STELROOT/po/$1 : "
	for f in "$group_dir"/$1-*.po "$group_dir"/$1.pot; do
		dest=${f##*/}
		dest=${dest#$group-}
		lang_code=${dest%.po}
		lang_code=${lang_code%.pot}
		dest="$1/$dest"
		cp "$f" "$dest"
		if [ $? -eq 0 ] && [ "$lang_code" != "$1" ]; then
			echo -n "$lang_code "
			processed_codes=(${processed_codes[*]} $lang_code)
		fi
	done
	echo ""
	echo ""

	# check the CMakeLists.txt file to ensure that all the translations we have updated
	# are going to be installed.
	tmp="$(mktemp)"
	if [ $? -ne 0 ]; then
		err "could not create tmp file to check CMakeLists.txt; SKIPPING check for $1"
		return
	fi

	sed -n '/GETTEXT_CREATE_TRANSLATIONS/,/^)/ { s/^.*TARGET//; s/)//; p}' "$1/CMakeLists.txt" \
		| tr " " "\n" \
		| grep -v "^[\t ]*$" > "$tmp"

	for code in ${processed_codes[*]}; do
		grep -q "^$code\$" "$tmp" || echo "WARNING language $code is not listed in CMakeLists.txt" >> "$log"
	done
	
	rm -f "$tmp"

	# Also check that the code is defined in the iso639-1.utf8 file
	for code in ${processed_codes[*]}; do
		tr '\t' ' ' < "$STELROOT/data/iso639-1.utf8" | grep -q "^$code" || echo "WARNING language $code is not defined in data/iso639-1.utf8" >> "$log"
	done
}

err () {
	echo "$@" 1>&2
}

errex () {
	e="${1:-1}"
	shift
	err "$@"
	exit $e
}

main "$@"

