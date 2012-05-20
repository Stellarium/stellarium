#!/usr/bin/perl
#
# read psrcat.db from ATNF Pulsar Catalogue and convert to JSON
# URL: http://www.atnf.csiro.au/research/pulsar/psrcat/download.html
#

$PSRCAT	= "./psrcat.db";
$JSON	= "../resources/catalog.json";

open (PSRCAT, "<$PSRCAT");
@catalog = <PSRCAT>;
close PSRCAT;

$psrcat = "";
foreach $s (@catalog) {
	$psrcat .= $s;
}

@cat = split("@",$psrcat);

open (JSON, ">$JSON");
print JSON "{\n";
print JSON "\t\"version\": \"0.2.0\",\n";
print JSON "\t\"shortName\": \"A catalogue of pulsars, based on ATNF Pulsar Catalogue v. 1.41\",\n";
print JSON "\t\"pulsars\":\n";
print JSON "\t{\n";

for ($i=0;$i<scalar(@cat)-1;$i++) {
	@lines = split("\n", $cat[$i]);
	$period = 0;
	$pderivative = 0;
	$bperiod = 0;
	$dmeasure = 0;
	$frequency = 0;
	$pfrequency = 0;
	$eccentricity = 0;
	$w50 = 0;
	$s400 = 0;
	$s600 = 0;
	$s1400 = 0;
	$distance = 0;
	$outRA = "";
	$outDE = "";
	$flag = 0;
	$notes = "";
	for ($j=0;$j<scalar(@lines);$j++) {
		if ($lines[$j] =~ /^PSRJ(\s+)J([\d]{4})([\+\-]{1})([\d]{2,4})([\w]{0,1})(\s+)/) {
			$name = $2.$3.$4.$5;
			$flag = 1;
		}

		if ($lines[$j] =~ /^RAJ(\s+)([\d\-\+\:\.]+)/) {
			($hour,$min,$sec) = split(":",$2);
			$sec += 0;
			$outRA = $hour."h".$min."m".$sec."s";
		}

		if ($lines[$j] =~ /^DECJ(\s+)([\d\-\+\:\.]+)/) {
			($deg,$min,$sec) = split(":",$2);
			$sec += 0;
			$outDE = $deg."d".$min."m".$sec."s";
		}

		if ($lines[$j] =~ /^P0(\s+)([\d\.]+)/) {
			$period = $2;
		}

		if ($lines[$j] =~ /^P1(\s+)([\d\.\-E]+)/) {
			$pderivative = $2;
		}

		if ($lines[$j] =~ /^PB(\s+)([\d\.]+)/) {
			$bperiod = $2;
		}

		if ($lines[$j] =~ /^F0(\s+)([\d\.]+)/) {
			$frequency = $2;
		}

		if ($lines[$j] =~ /^F1(\s+)([\d\.\-E]+)/) {
			$pfrequency = $2;
		}

		if ($lines[$j] =~ /^W50(\s+)([\d\.]+)/) {
			$w50 = $2;
		}

		if ($lines[$j] =~ /^S400(\s+)([\d\.]+)/) {
			$s400 = $2;
		}

		if ($lines[$j] =~ /^S600(\s+)([\d\.]+)/) {
			$s600 = $2;
		}

		if ($lines[$j] =~ /^S1400(\s+)([\d\.]+)/) {
			$s1400 = $2;
		}

		if ($lines[$j] =~ /^PX(\s+)([\d\.]+)/) {
			$parallax = $2;
		}

		if ($lines[$j] =~ /^DM(\s+)([\d\.]+)/) {
			$dmeasure = $2;
		}

		if ($lines[$j] =~ /^DIST_DM1(\s+)([\d\.\+\-]+)/) {
			$distance = $2;
		}

		if ($lines[$j] =~ /^ECC(\s+)([\d\.\-E]+)/) {
			$eccentricity = $2;
		}

		if ($lines[$j] =~ /^TYPE(\s+)([\w\,]+)/)
		{
			$notes = $2;
		}
	}

	$out  = "\t\t\"PSR J".$name."\":\n";
	$out .= "\t\t{\n";
	if ($parallax > 0) {
		$out .= "\t\t\t\"parallax\": ".$parallax.",\n";
	}
	if ($distance > 0) {
		$out .= "\t\t\t\"distance\": ".$distance.",\n";
	}
	if ($period > 0) {
		$out .= "\t\t\t\"period\": ".$period.",\n";
	}
	if ($bperiod > 0) {
		$out .= "\t\t\t\"bperiod\": ".$bperiod.",\n";
	}
	if ($eccentricity > 0) {
		$out .= "\t\t\t\"eccentricity\": ".$eccentricity.",\n";
	}
	if ($pderivative != 0) {
		$out .= "\t\t\t\"pderivative\": ".$pderivative.",\n";
	}
	if ($dmeasure > 0) {
		$out .= "\t\t\t\"dmeasure\": ".$dmeasure.",\n";
	}
	if ($frequency > 0) {
		$out .= "\t\t\t\"frequency\": ".$frequency.",\n";
	}
	if ($pfrequency != 0) {
		$out .= "\t\t\t\"pfrequency\": ".$pfrequency.",\n";
	}
	if ($w50 > 0) {
		$out .= "\t\t\t\"w50\": ".$w50.",\n";
	}
	if ($s400 > 0) {
		$out .= "\t\t\t\"s400\": ".$s400.",\n";
	}
	if ($s600 > 0) {
		$out .= "\t\t\t\"s600\": ".$s600.",\n";
	}
	if ($s1400 > 0) {
		$out .= "\t\t\t\"s1400\": ".$s1400.",\n";
	}
	if ($notes ne '')
	{
		$out .= "\t\t\t\"notes\": \"".$notes."\",\n";
	}
	$out .= "\t\t\t\"RA\": \"".$outRA."\",\n";
	$out .= "\t\t\t\"DE\": \"".$outDE."\"\n";
	$out .= "\t\t}";

	if ($i<scalar(@cat)-2) {
		$out .= ",";
	}
	
	if (($outRA ne '') && ($outDE ne '') && ($flag == 1)) {
	    print JSON $out."\n";
	}
}

print JSON "\t}\n}\n";

close JSON;
