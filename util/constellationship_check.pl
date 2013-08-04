#!/usr/bin/perl -n -a
#
# Quick sanity check for constellationship.fab files
#

#
# (C) Matthew Gates
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


$c = $F[0];
$n = $F[1]; 
@F = @F[2..$#F];

print "\n";
for($i=0; $i<$n; $i++) { 
	$a=$F[0]; 
	$b=$F[1]; 
	@F=@F[2..$#F]; 
	if ($a == $b) {
		$diag = "ERROR - same star at start and end of line";
	}
	else {
		$diag = "OK";
	}
	printf "%-3s %3d %-10s -> %-10s %s\n", $c, $i, $a, $b, $diag; 
}
