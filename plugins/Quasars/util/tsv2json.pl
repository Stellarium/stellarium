#!/usr/bin/perl
#
# read quasars.tsv from VizieR and convert to JSON
#

$TSV	= "./quasars.tsv";
$JSON	= "./catalog.json";

open (TSV, "<$TSV");
@catalog = <TSV>;
close TSV;

open (JSON, ">$JSON");
print JSON "{\n";
print JSON "\t\"version\": \"0.1.0\",\n";
print JSON "\t\"shortName\": \"A catalogue of quasars\",\n";
print JSON "\t\"quasars\":\n";
print JSON "\t{\n";

for ($i=15;$i<scalar(@catalog);$i++) {
	if ($catalog[$i] !~ /#/) {
		($name,$RA,$DE,$z,$Vmag,$bV,$Amag) = split(";", $catalog[$i]);

		($hour,$min,$sec) = split(" ",$RA);
		$outRA = $hour."h".$min."m".$sec."s";

		($deg,$min,$sec) = split(" ",$DE);
		$outDE = $deg."d".$min."m".$sec."s";

		$name =~ s/(\s{2,})//gi;
		$z =~ s/(\s+)//gi;
		$bV =~ s/(\s+)//gi;
		$Amag =~ s/(\s+)//gi;
		$bV =~ s/(\s+)//gi;

		# exclude quasar M 31
		if ($name ne 'M 31') {
			$out  = "\t\t\"".$name."\":\n";
			$out .= "\t\t{\n";
			$out .= "\t\t\t\"RA\": \"".$outRA."\",\n";
			$out .= "\t\t\t\"DE\": \"".$outDE."\",\n";
			$out .= "\t\t\t\"z\": ".$z.",\n";
			$out .= "\t\t\t\"Vmag\": ".$Vmag;
			if ($Amag ne '') {
				$out .= ",\n\t\t\t\"Amag\": ".$Amag;
			}
			if ($bV ne '') {
				$out .= ",\n\t\t\t\"bV\": ".$bV;
			}
			$out .= "\n\t\t}";

			if ($i<scalar(@catalog)-1) {
				$out .= ",";
			}

			print JSON $out."\n";
		}
	}
}

print JSON "\t}\n}";

close JSON;
