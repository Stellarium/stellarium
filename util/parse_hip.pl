#!/usr/bin/perl

# Creates hipparcos data file for use with Stellarium
# from source hip_main.dat data file available at
# http://cdsweb.u-strasbg.fr/viz-bin/Cat?I/239

# Copyright (C) 2005 Robert Spearman
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

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



print "\nConverting hip_main.dat to hipparcos.fab\n\n";
print "WARNING: This script has only been tested on x86 Linux.\n";
print "It may produce incorrect results on other platforms.\n\n";

sleep 2;

open(IN, "hip_main.dat") || die "Can't find hip_main.dat\n";
open(OUT, ">hipparcos.fab") || die "Can't write hipparcos.fab\n";

my $dmin = 1000000000000;
my $dmax = 0;

while(<IN>) {

	chomp;
	$_ =~ s/\|/\t/g;
	@f = split('\t', $_);

#	print("$f[1] : $f[3] : $f[4] : $f[5] : $f[76]\n");

	$hp = $f[1];
	$mag = $f[5];

	if($hp > $high) { $high = $hp; }  # data isn't contiguous...

	# ra and de in hms
	if( $f[3] =~ /(\-*)(\d\d) (\d\d) (\d\d\.\d\d)/ ) {
		$ra = $2 + $3/60 + $4/3600;
		if($1 eq "-") { $ra *= -1; }
	} else {
		print "error with ra on hp $hp\n";
		next;
	}

	if( $f[4] =~ /(\-*)(\d\d) (\d\d) (\d\d.\d)/ ) {
		$de = $2 + $3/60 + $4/3600;
		if($1) { $de *= -1; }
	} else {
		print "error with de on hp $hp\n";
		next;
	}

	$f[76] =~/^(.)(.)/;

	if( $1 eq "0") { 
		$sptype = 0; 
	} elsif($1 eq "B") {
		$sptype = 1;
	} elsif($1 eq "A") {
		$sptype = 2;
	} elsif($1 eq "F") { 
		$sptype = 3;
	} elsif($1 eq "G") { 
		$sptype = 4;
	} elsif($1 eq "K") { 
		$sptype = 5;
	} elsif($1 eq "M") { 
		$sptype = 6;
	} elsif($1 eq "R") { 
		$sptype = 7;
	} elsif($1 eq "S") { 
		$sptype = 8;
	} elsif($1 eq "N") { 
		$sptype = 9;
	} elsif($1 eq "W") {
		if($2 eq "N") {
			$sptype = 10;
		} else {
			$sptype = 11;
		}
	} else {
		$sptype = 12;
	}
	
	$spcode = $1;
	if($sptype == 11) {
		$spcode = "X";
	} elsif($sptype == 12) {
		$spcode = "?";
	}

#	print "$hp\t$ra\t$de\t$f[5]\t$f[76] ($sptype)\n";

	$x = 256*$mag;

#	printf( "%d\t%d\t%.4f\t%.4f\t%s\n", $hp, $x, $ra, $de, $spcode);

	# distance
	if($f[11]==0) {
		$ly = 0;
		print "No parallax for hp $hp\n";
	} else {
		$ly = abs(3.2616/($f[11]/1000));

		if($ly<$dmin) {$dmin = $ly;}
		if($ly>$dmax) {$dmax = $ly;}
	}


	$out{int($hp)} = pack("ffSCf", $ra, $de, $x, $sptype, $ly);
#	$out{int($hp)} = pack("ffSxC", $ra, $de, $x, $sptype);


}


# catalog size

print "Highest hp = $high\n";
print "ly min: $dmin\tlymax: $dmax\n";

print OUT pack("L", $high+1);  # hp numbers start at 1

# data isn't sorted or contiguous in source file!
for( $a=0; $a<=$high; $a++) {

	if($out{$a} eq "" ) {
		print OUT pack("xxxxxxxxxxxxxxx");
	} else {
		print OUT $out{$a};
	}

}
