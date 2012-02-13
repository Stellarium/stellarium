#!/usr/bin/perl
#
# read pulsars.tsv from VizieR and convert to JSON
#

$TSV	= "./pulsars.tsv";
$JSON	= "./catalog.json";

open (TSV, "<$TSV");
@catalog = <TSV>;
close TSV;

open (JSON, ">$JSON");
print JSON "{\n";
print JSON "\t\"version\": \"0.1.2\",\n";
print JSON "\t\"shortName\": \"A catalogue of pulsars\",\n";
print JSON "\t\"pulsars\":\n";
print JSON "\t{\n";

for ($i=0;$i<scalar(@catalog)-1;$i++) {
	if ($catalog[$i] =~ /^(\d+)/) {
		($RA,$DE,$name,$dist,$period,$ntype,$we,$w50,$s400,$s600,$s1400) = split(";", $catalog[$i]);

		($hour,$min,$sec) = split(" ",$RA);
		$outRA = $hour."h".$min."m".$sec."s";

		($deg,$min,$sec) = split(" ",$DE);
		$outDE = $deg."d".$min."m".$sec."s";

		$name =~ s/(\s{2,})//gi;
		$dist =~ s/(\s+)//gi;
		$period =~ s/(\s+)//gi;
		$ntype =~ s/(\s+)//gi;
		$we =~ s/(\s+)//gi;
		$w50 =~ s/(\s+)//gi;
		$s400 =~ s/(\s+)//gi;
		$s600 =~ s/(\s+)//gi;
		$s1400 =~ s/(\s+)//gi;

		$out  = "\t\t\"PSR J".$name."\":\n";
		$out .= "\t\t{\n";
		$out .= "\t\t\t\"RA\": \"".$outRA."\",\n";
		$out .= "\t\t\t\"DE\": \"".$outDE."\",\n";
		$out .= "\t\t\t\"distance\": ".$dist.",\n";
		$out .= "\t\t\t\"period\": ".$period.",\n";
		$out .= "\t\t\t\"ntype\": ".$ntype.",\n";
		$out .= "\t\t\t\"We\": ".$we.",\n";
		$out .= "\t\t\t\"w50\": ".$w50.",\n";
		$out .= "\t\t\t\"s400\": ".$s400.",\n";
		$out .= "\t\t\t\"s600\": ".$s600.",\n";
		$out .= "\t\t\t\"s1400\": ".$s1400."\n";
		$out .= "\t\t}";

		if ($i<scalar(@catalog)-2) {
			$out .= ",";
		}

		print JSON $out."\n";
	}
}

print JSON "\t}\n}\n";

close JSON;
