#!/usr/bin/perl
#
# read exoplanetData.csv from the 'Extrasolar Planets Encyclopaedia' at exoplanet.eu and convert to JSON
#

$CSV	= "./exoplanetData.csv";
$JSON	= "../resources/exoplanets.json";
$CATALOG_FORMAT_VERSION = 1;

open (CSV, "<$CSV");
@catalog = <CSV>;
close CSV;

open (JSON, ">$JSON");
print JSON "{\n";
print JSON "\t\"version\": \"".$CATALOG_FORMAT_VERSION."\",\n";
print JSON "\t\"shortName\": \"A catalogue of exoplanets\",\n";
print JSON "\t\"exoplanets\":\n";
print JSON "\t{\n";

for ($i=1;$i<scalar(@catalog);$i++) {
	$data = $catalog[$i];
	$data =~ s/\"//gi;
	($pname,$pmass,$pradius,$pperiod,$psemiax,$pecc,$pincl,$angdist,$ptrtime,$pstt,$pt,$pomega,$sdist,$sstype,$smass,$smetal,$sRA,$sDec,$sVmag,$sImag,$sHmag,$sJmag,$sKmag,$sradius,$sefftemp,$sage) = split(";", $data);

	($hour,$min,$sec) = split(" ",$sRA);
	$outRA = $hour."h".$min."m".$sec."s";

	($deg,$min,$sec) = split(" ",$sDec);
	$outDE = $deg."d".$min."m".$sec."s";
	
	$pname =~ s/^alpha/α/gi;
	$pname =~ s/^alf/α/gi;
	$pname =~ s/^beta/β/gi;
	$pname =~ s/^gamma/γ/gi;
	$pname =~ s/^delta/δ/gi;
	$pname =~ s/^epsilon/ε/gi;
	$pname =~ s/^eps/ε/gi;
	$pname =~ s/^zeta/ζ/gi;
	$pname =~ s/^theta/θ/gi;
	$pname =~ s/^eta/η/gi;
	$pname =~ s/^iota/ι/gi;
	$pname =~ s/^kappa/κ/gi;
	$pname =~ s/^lambda/λ/gi;
	$pname =~ s/^mu/μ/gi;
	$pname =~ s/^nu/ν/gi;
	$pname =~ s/^xi/ξ/gi;
	$pname =~ s/^ksi/ξ/gi;
	$pname =~ s/^omicron/ο/gi;
	$pname =~ s/^pi/π/gi;
	$pname =~ s/^rho/ρ/gi;
	$pname =~ s/^sigma/σ/gi;
	$pname =~ s/^tau/τ/gi;
	$pname =~ s/^upsilon/υ/gi;
	$pname =~ s/^ups/υ/gi;
	$pname =~ s/^phi/φ/gi;
	$pname =~ s/^chi/χ/gi;
	$pname =~ s/^psi/ψ/gi;
	$pname =~ s/^omega/ω/gi;
	
	if (($sRA ne '') && ($sDec ne '')) {
		$out  = "\t\t\"".$pname."\":\n";
		$out .= "\t\t{\n";
		if ($pmass ne '') {
			$out .= "\t\t\t\"mass\": ".$pmass.",\n";
		}
		if ($pradius ne '') {
			$out .= "\t\t\t\"radius\": ".$pradius.",\n";
		}
		if ($pperiod ne '') {
			$out .= "\t\t\t\"period\": ".$pperiod.",\n";
		}
		if ($psemiax ne '') {
			$out .= "\t\t\t\"semiAxis\": ".$psemiax.",\n";
		}
		if ($pecc ne '') {
			$out .= "\t\t\t\"eccentricity\": ".$pecc.",\n";
		}
		if ($pincl ne '') {
			$out .= "\t\t\t\"inclination\": ".$pincl.",\n";
		}
		$out .= "\t\t\t\"starRA\": \"".$outRA."\",\n";
		$out .= "\t\t\t\"starDE\": \"".$outDE."\"\n";
		$out .= "\n\t\t}";

		if ($i<scalar(@catalog)-1) {
			$out .= ",";
		}

		print JSON $out."\n";
	}
}

print JSON "\t}\n}";

close JSON;
