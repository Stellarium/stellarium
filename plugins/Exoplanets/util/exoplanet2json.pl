#!/usr/bin/perl
#
# read exoplanetData.csv from 'The Extrasolar Planets Encyclopaedia' at exoplanet.eu and convert to JSON
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
print JSON "\t\"shortName\": \"A catalogue of stars with exoplanets\",\n";
print JSON "\t\"stars\":\n";
print JSON "\t{\n";

for ($i=1;$i<scalar(@catalog);$i++) {
	$prevdata = $catalog[$i-1];
	$currdata = $catalog[$i];
	$prevdata =~ s/\"//gi;
	$currdata =~ s/\"//gi;
	
	@cpname = ();
	@ppname = ();
	(@cpname) = split(";",$currdata);
	(@ppname) = split(";",$prevdata);
	
	@pfname = ();
	@cfname = ();
	@pfname = split(" ",$ppname[0]);
	@cfname = split(" ",$cpname[0]);
	
	if (scalar(@cfname)==4) {
		$csname = $cfname[0]." ".$cfname[1]." ".$cfname[2];
		$pname = $cfname[3];
	} elsif (scalar(@cfname)==3) {
		$csname = $cfname[0]." ".$cfname[1];
		$pname = $cfname[2];
	} else {
		$csname = $cfname[0];
		$pname = $cfname[1];
	}
	if (scalar(@pfname)==4) {
		$psname = $pfname[0]." ".$pfname[1]." ".$pfname[2];
	} elsif (scalar(@pfname)==3) {
		$psname = $pfname[0]." ".$pfname[1];
	} else {
		$psname = $pfname[0];
	}
	
	($aname,$pmass,$pradius,$pperiod,$psemiax,$pecc,$pincl,$angdist,$ptrtime,$pstt,$pt,$pomega,$sdist,$sstype,$smass,$smetal,$sRA,$sDec,$sVmag,$sImag,$sHmag,$sJmag,$sKmag,$sradius,$sefftemp,$sage) = split(";", $currdata);

	($hour,$min,$sec) = split(" ",$sRA);
	$outRA = $hour."h".$min."m".$sec."s";

	($deg,$min,$sec) = split(" ",$sDec);
	$outDE = $deg."d".$min."m".$sec."s";
	
	$sname = $csname;

	$sname =~ s/^alpha/α/gi;
	$sname =~ s/^alf/α/gi;
	$sname =~ s/^beta/β/gi;
	$sname =~ s/^gamma/γ/gi;
	$sname =~ s/^delta/δ/gi;
	$sname =~ s/^epsilon/ε/gi;
	$sname =~ s/^eps/ε/gi;
	$sname =~ s/^zeta/ζ/gi;
	$sname =~ s/^theta/θ/gi;
	$sname =~ s/^eta/η/gi;
	$sname =~ s/^iota/ι/gi;
	$sname =~ s/^kappa/κ/gi;
	$sname =~ s/^lambda/λ/gi;
	$sname =~ s/^mu/μ/gi;
	$sname =~ s/^nu/ν/gi;
	$sname =~ s/^xi/ξ/gi;
	$sname =~ s/^ksi/ξ/gi;
	$sname =~ s/^omicron/ο/gi;
	$sname =~ s/^pi/π/gi;
	$sname =~ s/^rho/ρ/gi;
	$sname =~ s/^sigma/σ/gi;
	$sname =~ s/^tau/τ/gi;
	$sname =~ s/^upsilon/υ/gi;
	$sname =~ s/^ups/υ/gi;
	$sname =~ s/^phi/φ/gi;
	$sname =~ s/^chi/χ/gi;
	$sname =~ s/^psi/ψ/gi;
	$sname =~ s/^omega/ω/gi;
	
	if ($psname ne $csname) {
		# We have a star with first exoplanet
		
		if (($sRA ne '') && ($sDec ne '')) {
			$out  = "\t\t\"".$sname."\":\n";
			$out .= "\t\t{\n";
			$out .= "\t\t\t\"exoplanets\":\n";
			$out .= "\t\t\t[\n\t\t\t{\n";
			if ($pmass ne '') {
			    $out .= "\t\t\t\t\"mass\": \"".$pmass."\",\n";
			}
			if ($pradius ne '') {
				$out .= "\t\t\t\t\"radius\": \"".$pradius."\",\n";
			}
			if ($pperiod ne '') {
				$out .= "\t\t\t\t\"period\": \"".$pperiod."\",\n";
			}
			if ($psemiax ne '') {
				$out .= "\t\t\t\t\"semiAxis\": \"".$psemiax."\",\n";
			}
			if ($pecc ne '') {
				$out .= "\t\t\t\t\"eccentricity\": \"".$pecc."\",\n";
			}
			if ($pincl ne '') {
				$out .= "\t\t\t\t\"inclination\": \"".$pincl."\",\n";
			}
			if ($angdist ne '') {
				$out .= "\t\t\t\t\"angleDistance\": \"".$angdist."\",\n";
			}
			$out .= "\t\t\t\t\"planetName\": \"".$pname."\"\n";
			$out .= "\t\t\t}\n\t\t\t],\n";
			
			if ($sdist ne '') {
				$out .= "\t\t\t\"distance\": ".$sdist.",\n";
			}
			if ($sstype ne '') {
				$out .= "\t\t\t\"stype\": \"".$sstype."\",\n";
			}
			if ($smass ne '') {
				$out .= "\t\t\t\"smass\": ".$smass.",\n";
			}
			if ($smetal ne '') {
				$out .= "\t\t\t\"smetal\": ".$smetal.",\n";
			}
			if ($sVmag ne '') {
				$out .= "\t\t\t\"Vmag\": ".$sVmag.",\n";
			}
			if ($sradius ne '') {
				$out .= "\t\t\t\"sradius\": ".$sradius.",\n";
			}
			if ($sefftemp ne '') {
				$out .= "\t\t\t\"effectiveTemp\": ".$sefftemp.",\n";
			}
			$out .= "\t\t\t\"RA\": \"".$outRA."\",\n";
			$out .= "\t\t\t\"DE\": \"".$outDE."\"\n";
			$out .= "\t\t}";

			if ($i<scalar(@catalog)-1) {
				$out .= ",";
			}

			print JSON $out."\n";
		}
	}
}

print JSON "\t}\n}";

close JSON;
