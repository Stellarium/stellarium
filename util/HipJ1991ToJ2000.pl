#!/usr/bin/perl -w

#
# This tool read original Hipparcos catalog (J1991.25) + Hipparcos from VizieR (J2000.0)
# and update coordinates in original catalog.
#
# All catalogs can be found here: http://astro.uni-altai.ru/~aw/stellarium/hipparcos/
#

# Original Hipparcos catalog (1997; J1991.25)
$HIPmain	= "./hip_main.dat";
$HIPcom		= "./h_dm_com.dat";

# Hipparcos from VizieR (2013; J2000.0)
$HIPmainV	= "./hip_main_j2000.dat";
$HIPmainVa	= "./hip_main_j2000a.dat";
$HIPcomV	= "./h_dm_com_j2000.dat";

# Result
$HIPmainR	= "./hip_main_r.dat";
$HIPcomR	= "./h_dm_com_r.dat";

# Hipparcos main catalog
open(oHIP, "<$HIPmain");
@catalog = <oHIP>;
close oHIP;

open(cHIP, "<$HIPmainV");
@catalogV = <cHIP>;
close cHIP;

open(vHIP, "<$HIPmainVa");
@catalogA = <vHIP>;
close vHIP;

open(rHIP, ">$HIPmainR");
for($i=0;$i<scalar(@catalog);$i++) 
{
	@cat = split('\|',$catalog[$i]);
	@fix = split('\|',$catalogV[$i]);
	@fixa = split('\|',$catalogA[$i]);
	if ($cat[8] ne '            ' && $cat[9] ne '            ')
	{
		if ($fixa[0] ne '            ' && $fixa[1] ne '            ')
		{
			$cat[3] = substr($fixa[0],0,11); # RA
			$cat[4] = substr($fixa[1],0,11); # DE
		}
		$cat[8] = $fix[0]."00";
		$cat[9] = $fix[1]."00";
	}
	print rHIP join('|', @cat);
}
close rHIP;

# Hipparcos binary stars catalog
open(oHIP, "<$HIPcom");
@catalog = <oHIP>;
close oHIP;

open(cHIP, "<$HIPcomV");
@catalogV = <cHIP>;
close cHIP;

open(rHIP, ">$HIPcomR");
for($i=0;$i<scalar(@catalog);$i++) 
{
	@cat = split('\|',$catalog[$i]);
	@fix = split('\|',$catalogV[$i]);
	$cat[20] = $fix[0]."00"; # RA
	$cat[21] = $fix[1]."00"; # DE
	print rHIP join('|', @cat);
}
close rHIP;
