#!/usr/bin/perl

#
# Tool for generate catalog of quasars
#
# Copyright (C) 2012, 2018 Alexander Wolf
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 

#
# read quasars.tsv from VizieR and convert to JSON
#

$TSV	= "./quasars.tsv";
$JSON	= "./quasars.json";

open (TSV, "<$TSV");
@catalog = <TSV>;
close TSV;

open (JSON, ">$JSON");
print JSON "{\n";
print JSON "\t\"version\": \"1\",\n";
print JSON "\t\"shortName\": \"A catalogue of quasars\",\n";
print JSON "\t\"quasars\":\n";
print JSON "\t{\n";

for ($i=0;$i<scalar(@catalog);$i++) {
	if ($catalog[$i] =~ /^([a-zA-Z0-9]+)/) {
		($name,$RA,$DE,$f6,$f20,$z,$sp,$Vmag,$bV,$Amag) = split(";", $catalog[$i]);

		($hour,$min,$sec) = split(" ",$RA);
		$outRA = $hour."h".$min."m".$sec."s";

		($deg,$min,$sec) = split(" ",$DE);
		$outDE = $deg."d".$min."m".$sec."s";

		$name =~ s/(\s{2,})//gi;
		$name =~ s/(\s)$//gi;
		$z =~ s/(\s+)//gi;
		$f6 =~ s/(\s+)//gi;
		$f20 =~ s/(\s+)//gi;
		$sp =~ s/(\s+)//gi;
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
			if ($sp ne '') {
				$out .= ",\n\t\t\t\"sclass\": \"".$sp."\"";
			}
			if ($f6 ne '') {
				$out .= ",\n\t\t\t\"f6\": ".$f6;
			}
			if ($f20 ne '') {
				$out .= ",\n\t\t\t\"f20\": ".$f20;
			}
			$out .= "\n\t\t}";

			if ($i<scalar(@catalog)-2) {
				$out .= ",";
			}

			print JSON $out."\n";
		}
	}
}

print JSON "\t}\n}";

close JSON;
