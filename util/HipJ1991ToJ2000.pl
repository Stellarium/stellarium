#!/usr/bin/perl -w

#
# This tool read original Hipparcos catalog (J1991.25) + Hipparcos from VizieR (J2000.0)
# and update coordinates in original catalog.
#
# All catalogs can be found here: http://astro.uni-altai.ru/~aw/stellarium/hipparcos/
#
# Copyright (C) 2013 Alexander Wolf
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 
# 02110-1335, USA.


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
