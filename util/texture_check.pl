#!/usr/bin/perl
#
# (C) 2012 Matthew Gates
#


use strict;
use warnings;

use File::Basename;

my $gs_prog = basename $0;

if ($#ARGV<0) {
	usage(1);
}
else { 
	foreach (@ARGV) {
		if (/^(--help|-h)$/) { usage(0); }
	}
}

open(ID, '-|', 'identify', @ARGV) || die "cannot execute identify (perhaps ImageMagick is not installed?)\n";
while(<ID>) {
	my ($imgfile, undef, $dimensions, undef) = split(/\s+/);
	$imgfile =~ s/\[\d+\]$//;
	my ($w, $h) = split("x", $dimensions);

	if (log2($w) != int(log2($w)) || log2($h) != int(log2($h))) {
		printf "ERROR: non-integer power of 2 dimension for %-30s (%4d x %4d)\n", $imgfile, $w, $h, log2($w), log2($h);
	}
}

close(ID);

sub usage {
	my $el = shift || 0;
	print <<EOD;
Usage:
	$gs_prog imagefilename [imagefilename] ...

If any of the specified image files have dimensions which might be a problem for Stellarium,
they are printed with a suitable warning message.

Note: this program expects the ImageMagick binary "identify" to be availabale and in the
PATH.
EOD

	exit($el);
}

sub log2 {
	my $n = shift;
	return log($n)/log(2);
}

