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
print JSON "\t\"version\": \"0.1.0\",\n";
print JSON "\t\"shortName\": \"A catalogue of pulsars\",\n";
print JSON "\t\"pulsars\":\n";
print JSON "\t{\n";

for ($i=27;$i<scalar(@catalog)-1;$i++) {
	if ($catalog[$i] !~ /#/) {
		($RA,$DE,$name,$dist,$period,$ntype) = split(";", $catalog[$i]);

		($hour,$min,$sec) = split(" ",$RA);
		$outRA = $hour."h".$min."m".$sec."s";

		($deg,$min,$sec) = split(" ",$DE);
		$outDE = $deg."d".$min."m".$sec."s";

		$name =~ s/(\s{2,})//gi;
		$dist =~ s/(\s+)//gi;
		$period =~ s/(\s+)//gi;
		$ntype =~ s/(\s+)//gi;

		$out  = "\t\t\"".$name."\":\n";
		$out .= "\t\t{\n";
		$out .= "\t\t\t\"RA\": \"".$outRA."\",\n";
		$out .= "\t\t\t\"DE\": \"".$outDE."\",\n";
		$out .= "\t\t\t\"distance\": ".$dist.",\n";
		$out .= "\t\t\t\"period\": ".$period.",\n";
		$out .= "\t\t\t\"ntype\": ".$ntype;
		$out .= "\n\t\t}";

        if ($i<scalar(@catalog)-2) {
            $out .= ",";
        }

		print JSON $out."\n";
	}
}

print JSON "\t}\n}";

close JSON;
