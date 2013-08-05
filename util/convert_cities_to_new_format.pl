#!/usr/bin/perl

use warnings;
use strict;
use Getopt::Long;

=head1 old format

Apollo_11       USA     Mare_Tranquillitatis    0D40'26.7"N     23D28'22.7"E    0       0       1
Apollo_12       USA     Oceanus_Procellarum     3d0'44.604"S    23d25'17.652"W  0       0       1

=head1 new format

Mumbai  Maharashtra     in      R       12622,5 18.96N  72.82E  0
Buenos Aires    Distrito Federal / Buenos Aires ar      B       11928,4 34.61S  58.37W  0

=cut

my $planet = "";
my $typeset = "";

GetOptions (	'planet=s'	=> \$planet,
		'help'		=> sub { usage(0); },
		'type'		=> \$typeset,
) || usage(1);	

while(<>) {
	my ($cname, $cstate, $ccountry, $clat, $clon, $alt, $ctime, $showatzoom) = split(/\s+/);
	$cname =~ s/_/ /g;
	$cstate =~ s/_/ /g;
	$ccountry =~ s/_/ /g;
	$ccountry =~ s/^<>$//g;
	$clat = dms2dec($clat);
	$clon = dms2dec($clon);

	my $type;
	my $altitude = 0;
	my $population = 0;
	my $light_pollution;
	my $timezone = "";
	my $landscape;

	if ( $typeset eq "" ) {
		if ($planet eq "Earth" || $planet eq "") {
			$type = "N";
		}
		else {
			if ($cname =~ s/\s*\(crash\)\s*$//) {$type = "A";}
			elsif ($cname =~ s/\s*\(impact\)\s*$//) {$type = "I";}
			else {$type = "L";}
		}
	}
	else {
		$type = $typeset;
	}


	if ($planet eq "Earth" || $planet eq "") {
		$light_pollution = "";
	}
	else {
		$light_pollution = 0;
	}

	if ($planet eq "Earth" || $planet eq "") { $landscape = ""; }
	elsif ($planet eq "Moon") { $landscape = "moon"; }
	else { $landscape = ""; }

	# output format:
	#   - LocationName
	#   - Provice/State
	#   - Country(ISO Code/Full English Name)
	#   - Type code (C/B=Capital, R=Regional capital,N=Normal city, O=Observatory, L=lander, S=spacecraft (e.g. impactor))
	#   - Population (thousand)
	#   - Latitude (deg[N/S])
	#   - Longitude (deg[E/W])
	#   - Altitude (meters)
	#   - Light Pollution (0-9 Bortle scale, empty=auto)
	#   - TimeZone (empty=auto)
	#   - Planet (empty=Earth)
	#   - LandscapeKey (empty=auto)

	my $rec = sprintf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
		$cname,
		$cstate,
		$ccountry,
		$type,
		$population,
		$clat,
		$clon,
		$altitude,
		$light_pollution,
		$timezone,
		$planet,
		$landscape
	       );

	$rec =~ s/\s+$//;
	print $rec . "\n";

}


sub dms2dec {
	my ($deg, $min, $sec, $dir);
	if ($_[0] =~ /^(\d+)d(\d+)['m](\d+(\.\d+)?)["s]([NSEW])$/i) {
		($deg, $min, $sec, $dir) = ($1, $2, $3, $5);
	}
	elsif ($_[0] =~ /^(\d+)d((\d+)(\.\d+))['m]([NSEW])$/i) {
		($deg, $min, $dir) = ($1, $2, $4);
		$sec = 0;
	}
	elsif ($_[0] =~ /^(\d+)d?([NSEW])$/i) {
		($deg, $dir) = ($1, $2);
		$sec = 0;
		$min = 0;
	}
	else {
		warn "dms2dec: could not convert $_[0]\n";
		return 0;
	}
	my $dec = $deg + ( $min/60 ) + ( $sec/3600 );
	return sprintf("%.2f%s", $dec, $dir);
}

sub usage {
	system("pod2usage -verbose 1 $0");
	exit ($_[0] || 0);
}

